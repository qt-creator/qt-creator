TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    ast \
    semantic \
    lookup \
    preprocessor \
    findusages \
    typeprettyprinter \
    codeformatter \
    codegen
