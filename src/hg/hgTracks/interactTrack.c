/* interactTrack -- draw interaction between two genomic regions */

/* Copyright (C) 2018 The Regents of the University of California 
 * See README in this or parent directory for licensing information. */

#include "common.h"
#include "obscure.h"
#include "hgTracks.h"
#include "bigWarn.h"
#include "interact.h"
#include "interactUi.h"

static int interactTotalHeight(struct track *tg, enum trackVisibility vis)
/* calculate height of all the interactions being displayed */
{
if ( tg->visibility == tvDense)
    return  tl.fontHeight;
int min, max, deflt, current; 
cartTdbFetchMinMaxPixels(cart, tg->tdb, 
                                INTERACT_MINHEIGHT, INTERACT_MAXHEIGHT, atoi(INTERACT_DEFHEIGHT),
                                &min, &max, &deflt, &current);
return tg->height = current;
}

static Color interactItemColor(struct track *tg, void *item, struct hvGfx *hvg)
/* Return color to draw an interaction */
{
struct interact *inter = item;
if (tg->colorShades)
    return tg->colorShades[grayInRange(inter->score, 0, 1000)];

struct rgbColor itemRgb;
// There must be a better way...
itemRgb.r = (inter->color & 0xff0000) >> 16;
itemRgb.g = (inter->color & 0xff00) >> 8;
itemRgb.b = inter->color & 0xff;
return hvGfxFindColorIx(hvg, itemRgb.r, itemRgb.g, itemRgb.b);
}

void interactLoadItems(struct track *tg)
/* Load all interact items in region */
{
loadSimpleBedWithLoader(tg, (bedItemLoader)interactLoad);

if (tg->limitedVisSet)
    {
    // too many items to display
    // borrowed behaviors in bamTrack and vcfTrack
    // TODO BRANEY: make this behavior generic for bigBeds
    // (bigBedSelectRange)
    tg->drawItems = bigDrawWarning;
    tg->networkErrMsg = "Too many items in display (zoom in)"; 
    tg->totalHeight = bigWarnTotalHeight;
    return;
}

// filter and grayscale adjustment on score
char buf[1024];
safef(buf, sizeof buf, "%s.%s", tg->tdb->track, INTERACT_MINSCORE);
int minScore = cartUsualInt(cart, buf, INTERACT_DEFMINSCORE);
int scoreMin = atoi(trackDbSettingClosestToHomeOrDefault(tg->tdb, "scoreMin", "0"));
int scoreMax = atoi(trackDbSettingClosestToHomeOrDefault(tg->tdb, "scoreMax", "1000"));
struct interact *inter, *next, *filteredItems = NULL;
int count = slCount(tg->items);
for (inter = tg->items; inter; inter = next)
    {
    next = inter->next;
    if (inter->score < minScore)
        continue;
    if (tg->colorShades)
        {
        struct bed *bed = (struct bed *)inter;
        adjustBedScoreGrayLevel(tg->tdb, bed, scoreMin, scoreMax);
        }
    slAddHead(&filteredItems, inter);
    }
slReverse(&filteredItems);
// consider sorting by score/value so highest scored items draw last (on top)
if (slCount(filteredItems) != count)
    labelTrackAsFiltered(tg);
tg->items = filteredItems;
}

static void interactDrawItems(struct track *tg, int seqStart, int seqEnd,
        struct hvGfx *hvg, int xOff, int yOff, int width, 
        MgFont *font, Color color, enum trackVisibility vis)
/* Draw a list of interact structures. */
{
#define DRAW_LINE       0
#define DRAW_CURVE      1
#define DRAW_ELLIPSE    2

int draw = DRAW_LINE;
if (vis != tvDense)
    {
    char *drawMode = cartUsualStringClosestToHome(cart, tg->tdb, FALSE,
                                INTERACT_DRAW, INTERACT_DRAW_DEFAULT);
    if (sameString(drawMode, INTERACT_DRAW_CURVE))
        draw = DRAW_CURVE;
    else if (sameString(drawMode, INTERACT_DRAW_ELLIPSE))
        draw = DRAW_ELLIPSE;
    }
boolean isDirectional = interactUiDirectional(tg->tdb);

double scale = scaleForWindow(width, seqStart, seqEnd);
struct interact *inters = tg->items;
unsigned int maxWidth = 0;
struct interact *inter;
char buffer[1024];
char itemBuf[2048];

// Determine if there are mixed inter and intra-chromosomal 
// Suppress interchromosomal labels if they overlap
int nSame = 0, nOther = 0;
int prevLabelEnd = 0, prevLabelStart = 0;
char *prevLabel = 0;
boolean doOtherLabels = TRUE;
char *otherChrom = NULL;
for (inter=inters; inter; inter=inter->next)
    {
    int width = inter->chromEnd - inter->chromStart;
    if (width > maxWidth)
        maxWidth = width;
    }
for (inter=inters; inter; inter=inter->next)
    {
    otherChrom = interactOtherChrom(inter);
    if (otherChrom == NULL)
        nSame++;
    else
        {
        nOther++;
        if (!doOtherLabels)
            continue;
        int labelWidth = vgGetFontStringWidth(hvg->vg, font, otherChrom);
        // TODO: simplify now that center approach is abandoned
        int sx = ((inter->chromStart - seqStart) + .5) * scale + xOff; // x coord of center
        int labelStart = (double)sx - labelWidth/2;
        int labelEnd = labelStart + labelWidth - 1;
        if (labelStart <= prevLabelEnd && 
                !(labelStart == prevLabelStart && labelEnd == prevLabelEnd && 
                    sameString(otherChrom, prevLabel)))
            doOtherLabels = FALSE;
        prevLabelStart = labelStart;
        prevLabelEnd = labelEnd;
        prevLabel = otherChrom;
        }
    }
int fontHeight = vgGetFontPixelHeight(hvg->vg, font);
int otherHeight = (nOther) ? 3 * fontHeight : 0;
int sameHeight = (nSame) ? tg->height - otherHeight: 0;

// Draw items
for (inter=inters; inter; inter=inter->next)
    {
    char *otherChrom = interactOtherChrom(inter);
    safef(itemBuf, sizeof itemBuf, "%s", inter->name);
    struct dyString *ds = dyStringNew(0);
    if (isEmptyTextField(inter->name))
        {
        if (!isEmptyTextField(inter->exp))
            dyStringPrintf(ds, "%s ", inter->exp);
        if (otherChrom)
            dyStringPrintf(ds, "%s", otherChrom);
        else
            {
            char buf[4096];
            sprintLongWithCommas(buf, inter->chromEnd - inter->chromStart);
            dyStringPrintf(ds, "%s bp", buf);
            }
        }
    else
        dyStringPrintf(ds, "%s", inter->name);
    if (inter->score)
        dyStringPrintf(ds, " %d", inter->score);
    if (inter->value != 0.0)
        dyStringPrintf(ds, " %0.2f", inter->value);
    char *statusBuf = dyStringCannibalize(&ds);

    color = interactItemColor(tg, inter, hvg);
    if (vis == tvDense && interactOtherChrom(inter) && color == MG_BLACK)
        color = MG_MAGENTA;
    int peakColor = (color == MG_BLACK || tg->colorShades) ? MG_MAGENTA : MG_GRAY;
    
    // TODO: simplify by using start/end instead of center and width
    // This is a holdover from longRange track implementation
    unsigned lowStart, lowEnd, highStart, highEnd;
    if (otherChrom)
        {
        lowStart = highStart = inter->chromStart;
        lowEnd = highEnd = inter->chromEnd;
        }
    else if (inter->sourceStart < inter->targetStart)
        {
        lowStart = inter->sourceStart;
        lowEnd = inter->sourceEnd;
        highStart = inter->targetStart;
        highEnd = inter->targetEnd;
        }
    else
        {
        lowStart = inter->targetStart;
        lowEnd = inter->targetEnd;
        highStart = inter->sourceStart;
        highEnd = inter->sourceEnd;
        } 
    unsigned s = lowStart + ((double)(lowEnd - lowStart + .5) / 2);
    int sx = ((s - seqStart) + .5) * scale + xOff; // x coord of center (lower region)
    unsigned sw = lowEnd - lowStart;
    int sFootWidth = scale * (double)sw / 2;       // width in pixels of half foot (lower)
    if (sFootWidth == 0)
        sFootWidth = 1;

    unsigned e = highStart + (double)(highEnd - highStart + .5) / 2;
    int ex = ((e - seqStart) + .5) * scale + xOff;
    unsigned ew = highEnd - highStart;
    int eFootWidth = scale * (double)ew / 2;
    if (eFootWidth == 0)
        eFootWidth = 1;

    if (otherChrom)
        {
        // different chromosomes
        //      draw below same chrom items, if any
        unsigned yPos = 0;
        int height = 0;
        int yOffOther = yOff;
        if (tg->visibility == tvDense)
            {
            height = tg->height;
            }
        else
            {
            height = otherHeight/2;
            yOffOther = yOff + sameHeight;
            }
        yPos = yOffOther + height;

        // draw the foot
        hvGfxLine(hvg, sx - sFootWidth, yOffOther, sx + sFootWidth, yOffOther, color);

        // draw the vertical
        // TODO: modularize directional/non-directional draws
        if (differentString(inter->chrom, inter->sourceChrom) && isDirectional)
            hvGfxDottedLine(hvg, sx, yOffOther, sx, yPos, color, TRUE);
        else
            hvGfxLine(hvg, sx, yOffOther, sx, yPos, color);
        
        if (tg->visibility == tvFull)
            {
            // add map box to foot
            char *nameBuf = (inter->chromStart == inter->sourceStart ?      
                            inter->sourceName : inter->targetName);
            if (isEmptyTextField(nameBuf))
                nameBuf = statusBuf;
            mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, 
                            sx - sFootWidth, yOffOther, sFootWidth * 2, 4,
                            tg->track, itemBuf, nameBuf, NULL, TRUE, NULL);

            // add map box to vertical
            mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, sx - 2, yOffOther, 4, height,
                                   tg->track, itemBuf, statusBuf, NULL, TRUE, NULL);
            if (doOtherLabels)
                {
                safef(buffer, sizeof buffer, "%s", sameString(inter->chrom, inter->sourceChrom) ?
                                            inter->targetChrom : inter->sourceChrom);
                hvGfxTextCentered(hvg, sx, yPos + 2, 4, 4, MG_BLUE, font, buffer);
                int width = vgGetFontStringWidth(hvg->vg, font, buffer);

                // add mapBox to label
                mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, sx - width/2, yPos, 
                                width, fontHeight, tg->track, itemBuf, statusBuf, NULL, TRUE, NULL);
                }
            }
        continue;
        }

    // draw same chromosome interaction
    boolean sOnScreen = (s >= seqStart) && (s < seqEnd);
    boolean eOnScreen = (e >= seqStart) && (e < seqEnd);

    double interWidth = e - s;
    int peakHeight = (sameHeight - 15) * ((double)interWidth / maxWidth) + 10;
    int peak = yOff + peakHeight;
    if (tg->visibility == tvDense)
        peak = yOff + tg->height;

    if (sOnScreen)
        {
        // draw foot of lower region
        hvGfxLine(hvg, sx - sFootWidth, yOff, sx + sFootWidth, yOff, color);
        // draw vertical
        if (!eOnScreen || draw == DRAW_LINE)
            {
            if (inter->chromStart == inter->targetStart && isDirectional)
                hvGfxDottedLine(hvg, sx, yOff, sx, peak, color, TRUE);
            else
                hvGfxLine(hvg, sx, yOff, sx, peak, color);
            }
        }
    if (eOnScreen)
        {
        // draw foot of upper region
        hvGfxLine(hvg, ex - eFootWidth, yOff, ex + eFootWidth, yOff, color);

        // draw vertical
        if (!sOnScreen || draw == DRAW_LINE)
            {
            if (inter->chromStart == inter->targetStart && isDirectional)
                hvGfxDottedLine(hvg, ex, yOff, ex, peak, color, TRUE);
            else
                hvGfxLine(hvg, ex, yOff, ex, peak, color);
            }
        }
    if (tg->visibility == tvFull)
        {
        char *nameBuf = NULL;
        if (sOnScreen)
            {
            /* add mapbox to lower region */
            nameBuf = (inter->chromStart == inter->sourceStart ?      
                            inter->sourceName : inter->targetName);
            if (isEmptyTextField(nameBuf))
                nameBuf = statusBuf;
            hvGfxBox(hvg, sx-1, yOff, 3, 1, peakColor);
            hvGfxBox(hvg, sx, yOff, 1, 1, MG_WHITE);
            mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, 
                               sx - sFootWidth, yOff, sFootWidth * 2, 3,
                               tg->track, itemBuf, nameBuf, NULL, TRUE, NULL);

            }
        if (eOnScreen)
            {
            /* add mapbox to upper region */
            nameBuf = (inter->chromEnd == inter->targetEnd ?      
                            inter->targetName : inter->sourceName);
            if (isEmptyTextField(nameBuf))
                nameBuf = statusBuf;
            hvGfxBox(hvg, ex-1, yOff, 3, 1, peakColor);
            hvGfxBox(hvg, ex, yOff, 1, 1, MG_WHITE);
            mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, 
                            ex - eFootWidth, yOff, eFootWidth * 2, 3,
                            tg->track, itemBuf, nameBuf, NULL, TRUE, NULL);
            }
        if (sOnScreen && eOnScreen && draw != DRAW_LINE)
            {
            boolean isDashed = (inter->sourceStart > inter->targetStart);
            if (draw == DRAW_CURVE)
                {
                int peakX = ((ex - sx + 1) / 2) + sx;
                int peakY = peak + 30; // admittedly a hack (obscure how to define ypeak of curve)
                int maxY = hvGfxCurve(hvg, sx, yOff, peakX, peakY, ex, yOff, color, isDashed);
                // curve drawer does not use peakY as expected, so it returns actual max Y used
                // draw map box on peak
                hvGfxBox(hvg, peakX-1, maxY, 3, 1, peakColor);
                hvGfxBox(hvg, peakX, maxY, 1, 1, MG_WHITE);
                mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, peakX, maxY, 3, 1,
                               tg->track, itemBuf, nameBuf, NULL, TRUE, NULL);
                }
            else if (draw == DRAW_ELLIPSE)
                {
                int yLeft = yOff + peakHeight;
                int yTop = yOff - peakHeight;
                hvGfxEllipseDraw(hvg, sx, yLeft, ex, yTop, color, ELLIPSE_BOTTOM, isDashed);
                // draw map box on peak
                int maxY = peakHeight + yOff;
                int peakX = ((ex - sx + 1) / 2) + sx;
                hvGfxBox(hvg, peakX-1, maxY, 3, 1, peakColor);
                hvGfxBox(hvg, peakX, maxY, 1, 1, MG_WHITE);
                mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, peakX, maxY, 3, 1,
                               tg->track, itemBuf, nameBuf, NULL, TRUE, NULL);
                }
            }
        else
            {
            // draw link horizontal line between regions
            unsigned ePeak = eOnScreen ? ex : xOff + width;
            unsigned sPeak = sOnScreen ? sx : xOff;
            if (inter->sourceStart > inter->targetStart && isDirectional)
                hvGfxDottedLine(hvg, sPeak, peak, ePeak, peak, color, TRUE);
            else
                hvGfxLine(hvg, sPeak, peak, ePeak, peak, color);

            // map box on mid-point of horizontal line
            int xMap = sPeak + (double)(ePeak-sPeak)/2;
            int yMap = peak-1;
            hvGfxBox(hvg, xMap, peak-1, 1, 3, peakColor);
            hvGfxBox(hvg, xMap, peak, 1, 1, MG_WHITE);
            mapBoxHgcOrHgGene(hvg, inter->chromStart, inter->chromEnd, xMap-1, yMap, 3, 3,
                               tg->track, itemBuf, statusBuf, NULL, TRUE, NULL);
            }
        }
    }
}

void interactDrawLeftLabels(struct track *tg, int seqStart, int seqEnd,
    struct hvGfx *hvg, int xOff, int yOff, int width, int height,
    boolean withCenterLabels, MgFont *font,
    Color color, enum trackVisibility vis)
/* Override default */
{
}

void interactMethods(struct track *tg)
/* Interact track type methods */
{
tg->loadItems = interactLoadItems;
tg->drawItems = interactDrawItems;
tg->drawLeftLabels = interactDrawLeftLabels;
tg->totalHeight = interactTotalHeight;
tg->mapsSelf = TRUE;
}

void interactCtMethods(struct track *tg)
/* Interact track methods for custom track */
{
interactMethods(tg);
}

