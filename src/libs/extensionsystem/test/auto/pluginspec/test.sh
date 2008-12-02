# -- run the plugin test from this directory.

export LD_LIBRARY_PATH=../../../../../../lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=../../../../../../bin/QtCreator.app/Contents/PlugIns:$DYLD_LIBRARY_PATH # mac
exec ./test
