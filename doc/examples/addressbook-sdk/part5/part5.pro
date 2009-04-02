SOURCES   = addressbook.cpp \
            finddialog.cpp \
            main.cpp
HEADERS   = addressbook.h \
            finddialog.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part5
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part5.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part5
INSTALLS += target sources
