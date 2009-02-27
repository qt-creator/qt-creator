#version check qt
TOO_OLD_LIST=$$find(QT_VERSION, ^4\.[0-4])
count(TOO_OLD_LIST, 1) {
    message("Cannot build the Qt Creator with a Qt version that old:" $$QT_VERSION)
    error("Use at least Qt 4.5.")
}

include(doc/doc.pri)

TEMPLATE  = subdirs
CONFIG   += ordered

SUBDIRS = src

# for Qt Creator translations
QTC_BUILD_ROOT = $$PWD
QTC_INSTALL_TRANSLATIONS = $$PWD/translations

include(translations/translations.pri)
translations.path = $$QTC_INSTALL_TRANSLATIONS
translations.files = $$QTC_INSTALL_TRANSLATIONS/*.qm
