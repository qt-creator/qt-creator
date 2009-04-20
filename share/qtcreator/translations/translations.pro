IDE_BUILD_TREE = $$OUT_PWD/../../..
include(../../../qtcreator.pri)

TRANSLATIONS = de ja

# var, prepend, append
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

LUPDATE = $$targetPath($$[QT_INSTALL_PREFIX]/bin/lupdate) -locations relative -no-ui-lines
LRELEASE = $$targetPath($$[QT_INSTALL_PREFIX]/bin/lrelease)

TS_FILES = $$prependAll(TRANSLATIONS, $$PWD/qtcreator_,.ts)

contains(QT_VERSION, ^4\.[0-5]\..*):ts.commands = @echo This Qt version is too old for the ts target. Need Qt 4.6+.
else:ts.commands = (cd $$IDE_SOURCE_TREE && $$LUPDATE src -ts $$TS_FILES)
QMAKE_EXTRA_TARGETS += ts

contains(TEMPLATE, vc.*)|contains(TEMPLATE_PREFIX, vc):vcproj = 1

TEMPLATE = app
TARGET = phony_target2

updateqm.target = $$IDE_DATA_DIR/translations
updateqm.input = TS_FILES
updateqm.output = ${QMAKE_FILE_BASE}.qm
isEmpty(vcproj):updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

isEmpty(vcproj) {
    QMAKE_LINK = : IGNORE REST
    OBJECTS_DIR =
    win32:CONFIG -= embed_manifest_exe
} else {
    PHONY_DEPS = .
    phony_src.input = PHONY_DEPS
    phony_src.output = phony.c
    phony_src.commands = echo int main() { return 0; } > phony.c
    phony_src.CONFIG += combine
    QMAKE_EXTRA_COMPILERS += phony_src
}

qmfiles.files = $$prependAll(TRANSLATIONS, $$OUT_PWD/qtcreator_,.qm)
qmfiles.path = /share/qtcreator/translations
INSTALLS += qmfiles
