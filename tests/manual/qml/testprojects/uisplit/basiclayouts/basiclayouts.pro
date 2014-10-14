QT += qml quick
TARGET = basiclayouts
!no_desktop: QT += widgets

include(src/src.pri)
include(../shared/shared.pri)

OTHER_FILES += \
    main.qml \
    MainForm.qml \

RESOURCES += \
    resources.qrc
