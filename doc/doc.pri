build_online_docs: \
    DOC_FILES = $$PWD/fossil-online.qdocconf
else: \
    DOC_FILES = $$PWD/fossil.qdocconf

include($$IDE_SOURCE_TREE/docs.pri)
