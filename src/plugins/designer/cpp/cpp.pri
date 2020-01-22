DEFINES+=CPP_ENABLED

HEADERS+=$$PWD/formclasswizardpage.h \
    $$PWD/formclasswizarddialog.h \
    $$PWD/formclasswizard.h \
    $$PWD/formclasswizardparameters.h \
    $$PWD/newclasswidget.h

SOURCES+=$$PWD/formclasswizardpage.cpp \
    $$PWD/formclasswizarddialog.cpp  \
    $$PWD/formclasswizard.cpp \
    $$PWD/formclasswizardparameters.cpp \
    $$PWD/newclasswidget.cpp

FORMS+= \
    $$PWD/formclasswizardpage.ui \
    $$PWD/newclasswidget.ui
