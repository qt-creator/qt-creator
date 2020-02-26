build_online_docs: \
    DOC_FILES += $$IDE_DOC_FILES_ONLINE
else: \
    DOC_FILES += $$IDE_DOC_FILES

include(../docs.pri)

fixnavi.commands = \
    cd $$shell_path($$PWD) && \
    perl fixnavi.pl -Dqcmanual -Dqtquick src
QMAKE_EXTRA_TARGETS += fixnavi
