build_online_docs: \
    QTC_DOCS = $$PWD/fossil-online.qdocconf
else: \
    QTC_DOCS = $$PWD/fossil.qdocconf

include($$IDE_SOURCE_TREE/docs.pri)
