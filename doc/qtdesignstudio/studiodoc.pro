TEMPLATE = aux

# qtcreator.pro is a subdirs TEMPLATE which does not have debug_and_release
# so setup the same behavoir here for the current used aux TEMPLATE
CONFIG -= debug_and_release

# include(../qt-creator-defines.pri)
include(../../qtcreator.pri)

build_online_docs: \
    DOC_FILES = $$PWD/qtdesignstudio-online.qdocconf
else: \
    DOC_FILES = $$PWD/qtdesignstudio.qdocconf

include(../../docs.pri)
