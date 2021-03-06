# jaxQtlRaw.sql was originally generated by the autoSql program, which also 
# generated jaxQtlRaw.c and jaxQtlRaw.h.  This creates the database representation of
# an object which can be loaded and saved from RAM in a fairly 
# automatic way.

#Quantitative Trait Loci from Jackson Lab / Mouse Genome Informatics
CREATE TABLE jaxQtlRaw (
    mgiID varchar(255) not null,	# MGI ID
    name varchar(255) not null,	# Name of item
    description varchar(255) not null,	# MGI description
    chrom varchar(255) not null,	# chromosome
    flank1 varchar(255) not null,	# flank 1
    flank2 varchar(255) not null,	# flank 2
    peak varchar(255) not null,	# peak
    flank1Pos int unsigned not null,	# flank 1 position
    flank2Pos int unsigned not null,	# flank 2 position
    peakPos int unsigned not null,	# peak position
              #Indices
    KEY(mgiID),
    KEY(name)
);
