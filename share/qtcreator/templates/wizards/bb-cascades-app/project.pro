TEMPLATE = app

# Additional import path used to resolve QML modules in Creator's code model

LIBS += -lbbdata -lbb -lbbcascades
QT += declarative xml

# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += main.cpp \
    %ProjectName%.cpp

HEADERS += %ProjectName%.hpp

OTHER_FILES += bar-descriptor.xml \
    assets/main.qml \
    assets/SecondPage.qml


