TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    ast \
    codeformatter \
    findusages \
    lookup \
    preprocessor \
    semantic \
    typeprettyprinter \
    misc \
    c99 \
    cppselectionchanger\
    cxx11 \
    checksymbols \
    lexer \
    translationunit \
    fileiterationorder
