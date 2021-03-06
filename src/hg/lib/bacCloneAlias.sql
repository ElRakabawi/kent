# bacCloneAlias.sql was originally generated by the autoSql program, which also 
# generated bacCloneAlias.c and bacCloneAlias.h.  This creates the database representation of
# an object which can be loaded and saved from RAM in a fairly 
# automatic way.

#BAC clones associated STS aliases and Sanger STS names
CREATE TABLE bacCloneAlias (
    alias varchar(255) not null,	# STS aliases associated with BAC clones
    sangerName varchar(255) not null,	# Sanger STS name
              #Indices
    INDEX(alias),
    INDEX(sangerName)
);
