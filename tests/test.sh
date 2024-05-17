#!/bin/sh

# TODO: 
# - Create tmp directory where files are stored

cd "${0%/*}"/..
cd ./testfiles

echo "Zipping test dictionary..."
zip -j -r testdict.zip ./testdictionary/*

echo "Creating index..."
dictpopup-create -d ./

DP_DEBUG=1 DICTPOPUP_CONFIG=$(pwd) ../cli -d "$(pwd)" "面白い"
