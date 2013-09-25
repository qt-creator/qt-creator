TEMPLATE = app

LIBS += -lbbdata -lbb -lbbcascades
QT += declarative xml

SOURCES += main.cpp \
    %ProjectName%.cpp

HEADERS += %ProjectName%.hpp

OTHER_FILES += bar-descriptor.xml \
    assets/main.qml \
    assets/SecondPage.qml
