SOURCES   = addressbook.cpp \
            finddialog.cpp \
            main.cpp
HEADERS   = addressbook.h \
            finddialog.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part7
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part7.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part7
INSTALLS += target sources
