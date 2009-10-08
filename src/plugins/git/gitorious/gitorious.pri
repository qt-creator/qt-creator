QT += network
INCLUDEPATH+=$$PWD
DEPENDPATH+=$$PWD

HEADERS += $$PWD/gitoriousclonewizard.h \
           $$PWD/gitorioushostwizardpage.h \
           $$PWD/gitoriousrepositorywizardpage.h \
           $$PWD/gitoriousprojectwizardpage.h \
           $$PWD/gitoriousprojectwidget.h \
           $$PWD/gitorioushostwidget.h \
           $$PWD/gitorious.h

SOURCES += $$PWD/gitoriousclonewizard.cpp \
           $$PWD/gitorioushostwizardpage.cpp \
           $$PWD/gitoriousrepositorywizardpage.cpp \
           $$PWD/gitoriousprojectwizardpage.cpp \
           $$PWD/gitoriousprojectwidget.cpp \
           $$PWD/gitorioushostwidget.cpp \
           $$PWD/gitorious.cpp

FORMS +=   $$PWD/gitorioushostwidget.ui \
           $$PWD/gitoriousrepositorywizardpage.ui \
           $$PWD/gitoriousprojectwidget.ui
