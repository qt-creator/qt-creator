include(../../../qtcreator.pri)

LANGUAGES = cs de fr ja pl ru sl zh_CN zh_TW
# *don't* re-enable these without a prior rework
BAD_LANGUAGES = hu

# var, prepend, append
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

XMLPATTERNS = $$targetPath($$[QT_INSTALL_BINS]/xmlpatterns)
LUPDATE = $$targetPath($$[QT_INSTALL_BINS]/lupdate) -locations relative -no-ui-lines -no-sort
LRELEASE = $$targetPath($$[QT_INSTALL_BINS]/lrelease)
LCONVERT = $$targetPath($$[QT_INSTALL_BINS]/lconvert)

wd = $$replace(IDE_SOURCE_TREE, /, $$QMAKE_DIR_SEP)

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/qtcreator_,.ts)

MIME_TR_H = $$OUT_PWD/mime_tr.h
CUSTOMWIZARD_TR_H = $$OUT_PWD/customwizard_tr.h
QMLWIZARD_TR_H = $$OUT_PWD/qmlwizard_tr.h
EXTERNALTOOLS_TR_H = $$OUT_PWD/externaltools_tr.h

for(dir, $$list($$files($$IDE_SOURCE_TREE/src/plugins/*))):MIMETYPES_FILES += $$files($$dir/*.mimetypes.xml)
MIMETYPES_FILES = \"$$join(MIMETYPES_FILES, |)\"

for(dir, $$list($$files($$IDE_SOURCE_TREE/share/qtcreator/templates/wizards/*))):CUSTOMWIZARD_FILES += $$files($$dir/wizard.xml)
CUSTOMWIZARD_FILES = \"$$join(CUSTOMWIZARD_FILES, |)\"

for(dir, $$list($$files($$IDE_SOURCE_TREE/share/qtcreator/templates/qml/*))):QMLWIZARD_FILES += $$files($$dir/template.xml)
QMLWIZARD_FILES = \"$$join(QMLWIZARD_FILES, |)\"

for(file, $$list($$files($$IDE_SOURCE_TREE/src/share/qtcreator/externaltools/*))):EXTERNALTOOLS_FILES += $$files($$file)
EXTERNALTOOLS_FILES = \"$$join(EXTERNALTOOLS_FILES, |)\"

extract.commands += \
    $$XMLPATTERNS -output $$MIME_TR_H -param files=$$MIMETYPES_FILES $$PWD/extract-mimetypes.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$CUSTOMWIZARD_TR_H -param files=$$CUSTOMWIZARD_FILES $$PWD/extract-customwizards.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$QMLWIZARD_TR_H -param files=$$QMLWIZARD_FILES $$PWD/extract-qmlwizards.xq $$escape_expand(\\n\\t) \
    $$XMLPATTERNS -output $$EXTERNALTOOLS_TR_H -param files=$$EXTERNALTOOLS_FILES $$PWD/extract-externaltools.xq
QMAKE_EXTRA_TARGETS += extract

plugin_sources = $$files($$IDE_SOURCE_TREE/src/plugins/*)
win32:plugin_sources ~= s,\\\\,/,g
plugin_sources ~= s,^$$re_escape($$IDE_SOURCE_TREE/),,g$$i_flag
plugin_sources -= src/plugins/plugins.pro \
    src/plugins/helloworld \ # just an example
    # the following ones are dead
    src/plugins/qtestlib \
    src/plugins/snippets \
    src/plugins/regexp
sources = src/app src/libs $$plugin_sources src/shared share/qtcreator/qmldesigner \
          share/qtcreator/welcomescreen share/qtcreator/welcomescreen/widgets

files = $$files($$PWD/*_??.ts) $$PWD/qtcreator_untranslated.ts
for(file, files) {
    lang = $$replace(file, .*_([^/]*)\\.ts, \\1)
    v = ts-$${lang}.commands
    $$v = cd $$wd && $$LUPDATE $$sources $$MIME_TR_H $$CUSTOMWIZARD_TR_H $$QMLWIZARD_TR_H $$EXTERNALTOOLS_TR_H -ts $$file
    v = ts-$${lang}.depends
    $$v = extract
    QMAKE_EXTRA_TARGETS += ts-$$lang
}
ts-all.commands = cd $$wd && $$LUPDATE $$sources $$MIME_TR_H $$CUSTOMWIZARD_TR_H $$QMLWIZARD_TR_H $$EXTERNALTOOLS_TR_H -ts $$files
ts-all.depends = extract
QMAKE_EXTRA_TARGETS += ts-all

check-ts.commands = (cd $$replace(PWD, /, $$QMAKE_DIR_SEP) && perl check-ts.pl)
check-ts.depends = ts-all
QMAKE_EXTRA_TARGETS += check-ts

isEqual(QMAKE_DIR_SEP, /) {
    commit-ts.commands = \
        cd $$wd; \
        git add -N share/qtcreator/translations/*_??.ts && \
        for f in `git diff-files --name-only share/qtcreator/translations/*_??.ts`; do \
            $$LCONVERT -locations none -i \$\$f -o \$\$f; \
        done; \
        git add share/qtcreator/translations/*_??.ts && git commit
} else {
    commit-ts.commands = \
        cd $$wd && \
        git add -N share/qtcreator/translations/*_??.ts && \
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
CONFIG -= qt sdk separate_debug_info gdb_dwarf_index
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
qmfiles.path = $$QTC_PREFIX/share/qtcreator/translations
qmfiles.CONFIG += no_check_exist
INSTALLS += qmfiles
