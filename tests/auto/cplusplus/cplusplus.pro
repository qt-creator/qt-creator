TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    shared \
    ast \
    semantic \
    lookup \
    preprocessor \
    findusages \
    typeprettyprinter \
    codeformatter \
    codegen
