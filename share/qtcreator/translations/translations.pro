include(../../../qtcreator.pri)

LANGUAGES = de es fr it ja ru

# var, prepend, append
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

XMLPATTERNS = $$targetPath($$[QT_INSTALL_BINS]/xmlpatterns)
LUPDATE = $$targetPath($$[QT_INSTALL_BINS]/lupdate) -locations relative -no-ui-lines -no-sort
LRELEASE = $$targetPath($$[QT_INSTALL_BINS]/lrelease)

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/qtcreator_,.ts)

MIME_TR_H = $$IDE_DATA_PATH/translations/mime_tr.h

contains(QT_VERSION, ^4\.[0-5]\..*) {
    ts.commands = @echo This Qt version is too old for the ts target. Need Qt 4.6+.
} else {
    for(dir, $$list($$files($$IDE_SOURCE_TREE/src/plugins/*))):MIMETYPES_FILES += $$files($$dir/*.mimetypes.xml)
    MIMETYPES_FILES = \"$$join(MIMETYPES_FILES, \", \")\"
    QMAKE_SUBSTITUTES += extract-mimetypes.xq.in
    ts.commands += \
        $$XMLPATTERNS -output $$MIME_TR_H $$PWD/extract-mimetypes.xq && \
        (cd $$IDE_SOURCE_TREE && $$LUPDATE src $$MIME_TR_H -ts $$TRANSLATIONS) && \
        $$QMAKE_DEL_FILE $$MIME_TR_H
}
QMAKE_EXTRA_TARGETS += ts

TEMPLATE = app
TARGET = phony_target2
CONFIG -= qt
QT =
LIBS =

updateqm.input = TRANSLATIONS
updateqm.output = $$IDE_DATA_PATH/translations/${QMAKE_FILE_BASE}.qm
isEmpty(vcproj):updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

isEmpty(vcproj) {
    QMAKE_LINK = @: IGNORE THIS LINE
    OBJECTS_DIR =
    win32:CONFIG -= embed_manifest_exe
} else {
    CONFIG += console
    PHONY_DEPS = .
    phony_src.input = PHONY_DEPS
    phony_src.output = phony.c
    phony_src.variable_out = GENERATED_SOURCES
    phony_src.commands = echo int main() { return 0; } > phony.c
    phony_src.name = CREATE phony.c
    phony_src.CONFIG += combine
    QMAKE_EXTRA_COMPILERS += phony_src
}

qmfiles.files = $$prependAll(LANGUAGES, $$OUT_PWD/qtcreator_,.qm)
qmfiles.path = /share/qtcreator/translations
INSTALLS += qmfiles
