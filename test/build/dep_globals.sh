#!/bin/bash

# author        Oliver Blaser
# date          03.11.2024
# copyright     MIT - Copyright (c) 2024 Oliver Blaser

prjName="curl-thread-test"
prjDisplayName="System and unit test of curl-thread"
prjBinName=$prjName
prjDirName=$prjName
repoDirName="curl-thread"
copyrightYear="2025"

versionstr=$(head -n 1 dep_vstr.txt)
versionstrDeb=$(echo "$versionstr" | tr - "~") # convert semver pre-release delimiter to debian compatible char

function ptintTitle()
{
    if [ "$2" = "" ]
    then
        echo "  --=====#   $1   #=====--"
    else
        echo -e "\033[3$2m  --=====#   \033[9$2m$1\033[3$2m   #=====--\033[39m"
    fi
}

# pass output filename as argument
function writeReadmeFile()
{
    echo ""                                 > $1
    echo "${prjDisplayName} v${versionstr}" >> $1
    echo ""                                 >> $1
    echo "MIT - Copyright (c) ${copyrightYear} Oliver Blaser" >> $1
    echo ""                                 >> $1
    echo "https://github.com/oblaser/${repoDirName}" >> $1
}
