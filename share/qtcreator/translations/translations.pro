include(../../../qtcreator.pri)

LANGUAGES = cs de fr ja pl ru

# var, prepend, append
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

XMLPATTERNS = $$targetPath($$[QT_INSTALL_BINS]/xmlpatterns)
LUPDATE = $$targetPath($$[QT_INSTALL_BINS]/lupdate) -locations relative -no-ui-lines -no-sort
LRELEASE = $$targetPath($$[QT_INSTALL_BINS]/lrelease)
LCONVERT = $$targetPath($$[QT_INSTALL_BINS]/lconvert)

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/qtcreator_,.ts)

MIME_TR_H = $$OUT_PWD/mime_tr.h
CUSTOMWIZARD_TR_H = $$OUT_PWD/customwizard_tr.h

for(dir, $$list($$files($$IDE_SOURCE_TREE/src/plugins/*))):MIMETYPES_FILES += $$files($$dir/*.mimetypes.xml)
MIMETYPES_FILES = \"$$join(MIMETYPES_FILES, |)\"

for(dir, $$list($$files($$IDE_SOURCE_TREE/share/qtcreator/templates/wizards/*))):CUSTOMWIZARD_FILES += $$files($$dir/wizard.xml)
CUSTOMWIZARD_FILES = \"$$join(CUSTOMWIZARD_FILES, |)\"

extract.commands += \
    $$XMLPATTERNS -output $$MIME_TR_H -param files=$$MIMETYPES_FILES $$PWD/extract-mimetypes.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$CUSTOMWIZARD_TR_H -param files=$$CUSTOMWIZARD_FILES $$PWD/extract-customwizards.xq
QMAKE_EXTRA_TARGETS += extract

files = $$files($$PWD/*_??.ts) $$PWD/qtcreator_untranslated.ts
for(file, files) {
    lang = $$replace(file, .*_([^/]*)\\.ts, \\1)
    v = ts-$${lang}.commands
    $$v = cd $$IDE_SOURCE_TREE && $$LUPDATE src share/qtcreator/qmldesigner $$MIME_TR_H $$CUSTOMWIZARD_TR_H -ts $$file
    v = ts-$${lang}.depends
    $$v = extract
    QMAKE_EXTRA_TARGETS += ts-$$lang
}
ts-all.commands = cd $$IDE_SOURCE_TREE && $$LUPDATE src share/qtcreator/qmldesigner $$MIME_TR_H $$CUSTOMWIZARD_TR_H -ts $$files
ts-all.depends = extract
QMAKE_EXTRA_TARGETS += ts-all

check-ts.commands = (cd $$PWD && perl check-ts.pl)
check-ts.depends = ts-all
QMAKE_EXTRA_TARGETS += check-ts

isEqual(QMAKE_DIR_SEP, /) {
    commit-ts.commands = \
        cd $$IDE_SOURCE_TREE; \
        for f in `git diff-files --name-only share/qtcreator/translations/*_??.ts`; do \
            $$LCONVERT -locations none -i \$\$f -o \$\$f; \
        done; \
        git add share/qtcreator/translations/*_??.ts && git commit
} else {
    wd = $$replace(IDE_SOURCE_TREE, /, \\)
    commit-ts.commands = \
        cd $$wd && \
        for /f usebackq %%f in (`git diff-files --name-only share/qtcreator/translations/*_??.ts`) do \
            $$LCONVERT -locations none -i %%f -o %%f $$escape_expand(\\n\\t) \
        cd $$wd && git add share/qtcreator/translations/*_??.ts && git commit
}
QMAKE_EXTRA_TARGETS += commit-ts

ts.commands = \
    @echo \"The \'ts\' target has been removed in favor of more fine-grained targets.\" && \
    echo \"Use \'ts-<lang>\' instead. To add a language, use \'ts-untranslated\',\" && \
    echo \"rename the file and re-run \'qmake\'.\"
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
qmfiles.CONFIG += no_check_exist
INSTALLS += qmfiles
