INCLUDEPATH+=$$PWD

DEFINES+=CPP_ENABLED

HEADERS+=$$PWD/formclasswizardpage.h \
    $$PWD/formclasswizarddialog.h \
    $$PWD/formclasswizard.h \
    $$PWD/formclasswizardparameters.h \
    $$PWD/cppsettingspage.h

SOURCES+=$$PWD/formclasswizardpage.cpp \
    $$PWD/formclasswizarddialog.cpp  \
    $$PWD/formclasswizard.cpp \
    $$PWD/formclasswizardparameters.cpp \
    $$PWD/cppsettingspage.cpp

FORMS+=$$PWD/formclasswizardpage.ui \
$$PWD/cppsettingspagewidget.ui
