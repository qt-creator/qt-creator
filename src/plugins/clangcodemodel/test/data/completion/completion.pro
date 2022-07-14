QT += widgets

INCLUDEPATH += $$PWD/exampleIncludeDir

SOURCES = \
    classAndConstructorCompletion.cpp \
    completionWithProject.cpp \
    constructorCompletion.cpp \
    dotToArrowCorrection.cpp \
    doxygenKeywordsCompletion.cpp \
    functionAddress.cpp \
    functionCompletion.cpp \
    functionCompletionFiltered2.cpp \
    functionCompletionFiltered.cpp \
    globalCompletion.cpp \
    includeDirectiveCompletion.cpp \
    main.cpp \
    mainwindow.cpp \
    memberCompletion.cpp \
    membercompletion-friend.cpp \
    membercompletion-inside.cpp \
    membercompletion-outside.cpp \
    noDotToArrowCorrectionForFloats.cpp \
    preprocessorKeywordsCompletion.cpp \
    preprocessorKeywordsCompletion2.cpp \
    preprocessorKeywordsCompletion3.cpp \
    privateFuncDefCompletion.cpp \
    signalCompletion.cpp

QMAKE_CXXFLAGS += -broken

HEADERS = mainwindow.h
FORMS = mainwindow.ui
