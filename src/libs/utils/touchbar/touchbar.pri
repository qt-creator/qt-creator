HEADERS += $$PWD/touchbar.h

macos {
    HEADERS += \
        $$PWD/touchbar_mac_p.h \
        $$PWD/touchbar_appdelegate_mac_p.h

    OBJECTIVE_SOURCES += \
        $$PWD/touchbar_mac.mm \
        $$PWD/touchbar_appdelegate_mac.mm

    LIBS += -framework Foundation -framework AppKit
} else {
    SOURCES += $$PWD/touchbar.cpp
}

