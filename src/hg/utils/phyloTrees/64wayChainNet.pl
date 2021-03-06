#!/usr/bin/env perl
# chainNet.pl - output chainNet.ra definitions in phylogentic order

#	$Id: chainNet.pl,v 1.5 2010/04/01 17:04:12 hiram Exp $

use strict;
use warnings;
use FindBin qw($Bin);
use lib "$Bin";

# new genomes since the 46-way construction.  This will keep
#	the priority numbers the same for the previous 46 and place
#	these new ones in between the previous ones.
my $newAssemblies = "nomLeu hetGla susScr oviAri ailMel sarHar melGal allMis croPor latCha oreNil chrPic gadMor saiBol melUnd triMan geoFor cerSim";
my $newAssemblyOffset = 5;
# second level of adding new assemblies, these are right next to another
# new one, they need half of the offset as the above
my $offsetNway = 10;
my $reroot = "$Bin/rerootTree.pl";
my $hgsql = "hgsql";
my $home = $ENV{'HOME'};
my $dissectTree = "$home/kent/src/hg/utils/phyloTrees/64way.dissect.txt";
my $specificOffsets = "allMis croPor melUnd";
my %specificOffsets = (
"croPor" => 561.3,
"allMis" => 562.3,
"melUnd" => 563.3,
);

my $argc = scalar(@ARGV);

if ($argc != 1) {
    printf STDERR "chainNet.pl - output chainNet.ra definitions in phylogenetic order\n";
    printf STDERR "usage:\n    chainNet.pl <db>\n";
    printf STDERR "<db> - an existing UCSC database\n";
    printf STDERR "will be using commands: rerootTree.pl and hgsql\n";
    printf STDERR "therefore expecting to find:\n";
    printf STDERR "$reroot\n";
    printf STDERR "$dissectTree\n";
    exit 255;
}

my $initialPrio = 200.3;
my $db = shift;
my $dbCount = 0;
my %priorities;
my $priority = $initialPrio - $offsetNway;
my $stripName = $db;
$stripName =~ s/[0-9]+$//;

my $actualTreeName = "notFound";

# given an arbitrary database name, find corresponding database name
# that is actually in the phylo tree
open (FH, "grep name $dissectTree | sort -u | sed -e \"s/.*name = '//; s/'.*//;\"|") or die "can not read $dissectTree";
while (my $name = <FH>) {
    chomp $name;
    my $treeName = $name;
    $name =~ s/[0-9]+$//;
    if ($name eq $stripName) {
	$actualTreeName = $treeName;
	last;
    }
}

# verify we found one
if ($actualTreeName eq "notFound") {
    printf "ERROR: specified db '$db' is not like one of the databases in the phylo tree\n";
    printf `grep name $dissectTree | sort -u | sed -e "s/.*name = '//; s/'.*//;" | xargs echo`, "\n";
    exit 255;
}

my $nextOffset = $offsetNway;
# reroot the tree to that database name
open (FH, "$reroot $actualTreeName $dissectTree 2> /dev/null|") or die "can not run rerootTree.pl";
while (my $line = <FH>) {
    chomp $line;
    $line =~ s/[0-9]+$//;
    if (!exists($priorities{lcfirst($line)})) {
	if ($newAssemblies =~ m/$line/) {
            my $kludgePriority = $priority;
            if ($specificOffsets =~ m/$line/) {
                my $name = $line;
                $name =~ s/[0-9]+$//;
                $kludgePriority = $specificOffsets{$name};
            } else {
                $kludgePriority += $newAssemblyOffset;
                $priority += $newAssemblyOffset;
                $nextOffset = $newAssemblyOffset;
            }
#            printf STDERR "%s: %s\n", lcfirst($line), $kludgePriority;
	    $priorities{lcfirst($line)} = $kludgePriority;
	} else {
	    $priority += $nextOffset;
#            printf STDERR "%s: %s\n", lcfirst($line), $priority;
	    $priorities{lcfirst($line)} = $priority;
            $nextOffset = $offsetNway;
        }
    }
}
close (FH);

my $chainCount = 0;
my %orderChainNet;

# fetch all chain tables from the given database and for ones that are
# in the phylo tree, output their priority
open (FH, "hgsql -e 'show tables;' $db | grep 'chain.*Link' | egrep -i -v 'self|chainNet' | sed -e 's/^.*_chain/chain/' | sort -u|") or
	die "can not run hgsql 'show tables' on $db";
while (my $tbl = <FH>) {
    chomp $tbl;
    $tbl =~ s/^chain//;
    $tbl =~ s/Link$//;
    my $track = $tbl;
    my $stripDb = $tbl;
    $stripDb =~ s/[0-9]+$//;
    $stripDb =~ s/V17e$//;
    $stripDb =~ s/Poodle$//;
    if (defined($stripDb) && length($stripDb) > 0) {
	if (exists($priorities{lcfirst($stripDb)})) {
	    my $priority = $priorities{lcfirst($stripDb)};
	    $orderChainNet{$track} = $priority;
	} else {
	    printf STDERR "warning: not in phylo tree: $tbl ($stripDb)\n";
	}
    }
}
close (FH);

# print out the priorities in order by priority, lowest first
foreach my $track (sort { $orderChainNet{$a} <=> $orderChainNet{$b} }
	keys %orderChainNet) {
    my $priority = $orderChainNet{$track};
    printf "track chainNet%s override\n", $track;
    printf "priority %s\n\n", $priority;
}
