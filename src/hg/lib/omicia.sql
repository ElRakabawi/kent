# omicia.sql was originally generated by the autoSql program, which also 
# generated omicia.c and omicia.h.  This creates the database representation of
# an object which can be loaded and saved from RAM in a fairly 
# automatic way.

#table for OMICIA auto-generated data
CREATE TABLE omiciaAuto (
    bin smallint unsigned not null,	# A field to speed indexing
    chrom varchar(255) not null,	# Chromosome
    chromStart int unsigned not null,	# Start position in chrom
    chromEnd int unsigned not null,	# End position in chrom
    name varchar(255) not null,	# ID for this mutation
    score int unsigned not null,	# confidence score
    strand char(1) not null,	# + or -
              #Indices
    INDEX(bin)
);

#table for OMICIA hand curated data
CREATE TABLE omiciaHand (
    bin smallint unsigned not null,	# A field to speed indexing
    chrom varchar(255) not null,	# Chromosome
    chromStart int unsigned not null,	# Start position in chrom
    chromEnd int unsigned not null,	# End position in chrom
    name varchar(255) not null,	# ID for this mutation
    score int unsigned not null,	# confidence score
    strand char(1) not null,	# + or -
              #Indices
    INDEX(bin)
);

#links
CREATE TABLE omiciaLink (
    id varchar(255) not null,	# id into the omicia composite table
    attrType varchar(55) not null,	# attribute type
    raKey varchar(255) not null,	# key into .ra file on how to do link
    acc varchar(255) not null,	# accession or id used by link
    displayVal varchar(255) not null,	# value to display if different from acc
              #Indices
    INDEX(id, attrType)
);

#attributes
CREATE TABLE omiciaAttr (
    id varchar(255) not null,   # id into the omicia composite table
    attrType varchar(255) not null,     # attribute type, label
    attrVal varchar(255) not null,      # value for this attribute
              #Indices
    INDEX(id)
);

