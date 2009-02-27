defineReplace(prependAll) {
    prepend = $$1
    arglist = $$2
    append  = $$3
    for(a,arglist) {
      result += $${prepend}$${a}$${append}
    }
    return ($$result)
}

defineReplace(fixPath) {
WIN {
    return ($$replace($$1, /, \))
} ELSE {
    return ($$1)
}
}

LUPDATE = $$fixPath($$[QT_INSTALL_PREFIX]/bin/lupdate) -locations relative -no-ui-lines
LRELEASE = $$fixPath($$[QT_INSTALL_PREFIX]/bin/lrelease)


###### Qt Creator

QTC_TS = de fr zh_CN untranslated ar es iw ja_JP pl pt ru sk sv uk zh_TW

ts.commands = (cd $$QTC_BUILD_ROOT && \
               $$LUPDATE share src \
                         -ts $$prependAll($$QTC_INSTALL_TRANSLATIONS/qtcreator_,$$QTC_TS,.ts))

qm.commands = $$LRELEASE $$prependAll($$QTC_INSTALL_TRANSLATIONS/qtcreator_,$$QTC_TS,.ts)

QMAKE_EXTRA_TARGETS += ts qm
