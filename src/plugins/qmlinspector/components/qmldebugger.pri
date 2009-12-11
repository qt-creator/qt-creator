QT += network declarative
contains(QT_CONFIG, opengles2)|contains(QT_CONFIG, opengles1): QT += opengl

# Input
HEADERS += $$PWD/canvasframerate.h \
           $$PWD/watchtable.h \
           $$PWD/objecttree.h \
           $$PWD/objectpropertiesview.h \
           $$PWD/expressionquerywidget.h

SOURCES += $$PWD/canvasframerate.cpp \
           $$PWD/watchtable.cpp \
           $$PWD/objecttree.cpp \
           $$PWD/objectpropertiesview.cpp \
           $$PWD/expressionquerywidget.cpp

