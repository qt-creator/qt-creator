QT = qtsaferenderer qsrplatformadaptation

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \\
        %{CppFileName}


# List of language codes that your application supports. For example, SAFE_LANGUAGES = en fi.
#SAFE_LANGUAGES = en

# List of translation file names excluding the language code. For example, SAFE_TRANSLATION = $$PWD/safeui.
#SAFE_TRANSLATION = $$PWD/safeui

# List of translation file names including the language code. There must be one file
# for each language listed in SAFE_LANGUAGES. For example, TRANSLATIONS += safeui_en.ts safeui_fi.ts.
#TRANSLATIONS += safeui_en.ts

# You can use an lupdate_only{...} conditional statement to specify the QML files that contain texts.
#lupdate_only {
#    SOURCES += main.qml
#}


RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = $$PWD/imports

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =


CONFIG += qtsaferenderer exceptions
SAFE_QML = $$PWD/main.qml
SAFE_LAYOUT_PATH = $$PWD/layoutData
SAFE_RESOURCES += safeasset.qrc

!cross_compile: DEFINES += HOST_BUILD
!cross_compile: QT += widgets quick svg

DISTFILES += main.qml
