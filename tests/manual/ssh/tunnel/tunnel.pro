include(../ssh.pri)

TARGET =tunnel
SOURCES = \
    main.cpp \
    argumentscollector.cpp \
    directtunnel.cpp \
    forwardtunnel.cpp
HEADERS = \
    argumentscollector.h \
    directtunnel.h \
    forwardtunnel.h
