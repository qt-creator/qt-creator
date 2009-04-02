SOURCES   = addressbook.cpp \
            main.cpp
HEADERS   = addressbook.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part2
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part2.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tutorials/addressbook/part2
INSTALLS += target sources
