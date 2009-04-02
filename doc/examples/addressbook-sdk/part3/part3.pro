SOURCES = addressbook.cpp \
          main.cpp
HEADERS = addressbook.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part3
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part3.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part3
INSTALLS += target sources
