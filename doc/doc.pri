build_online_docs: \
    DOC_FILES += $$PWD/qtcreator/qtcreator-online.qdocconf $$PWD/qtcreatordev/qtcreator-dev-online.qdocconf
else: \
    DOC_FILES += $$PWD/qtcreator/qtcreator.qdocconf $$PWD/qtcreatordev/qtcreator-dev.qdocconf

include(../docs.pri)

fixnavi.commands = \
    cd $$shell_path($$PWD) && \
    perl fixnavi.pl -Dqcmanual -Dqtquick src
QMAKE_EXTRA_TARGETS += fixnavi
