macx {
    SNIPPETS.path = Contents/Resources
    SNIPPETS.files = $$PWD/qtcreator/snippets
    TEMPLATES.path = Contents/Resources
    TEMPLATES.files = $$PWD/qtcreator/templates
    DESIGNER.path = Contents/Resources
    DESIGNER.files = $$PWD/qtcreator/designer
    SCHEMES.path = Contents/Resources
    SCHEMES.files = $$PWD/qtcreator/schemes
    GDBDEBUGGER.path = Contents/Resources
    GDBDEBUGGER.files = $$PWD/qtcreator/gdbmacros
    RUNINTERMINAL.path = Contents/Resources
    RUNINTERMINAL.files = $$PWD/qtcreator/runInTerminal.command
    QMAKE_BUNDLE_DATA += SNIPPETS TEMPLATES DESIGNER SCHEMES GDBDEBUGGER RUNINTERMINAL
    QMAKE_INFO_PLIST = $$PWD/qtcreator/Info.plist
}

win32|linux-* {
    # make sure the resources are in place
    !exists($$OUT_PWD/app.pro) {
        unix:SEPARATOR = ;
        win32:SEPARATOR = &
        # we are shadow build
        COPYSRC = snippets \
                   templates \
                   designer \
                   schemes \
                   gdbmacros
        COPYDEST = $${OUT_PWD}/../../share/qtcreator
        win32:COPYDEST ~= s|/+|\|
        QMAKE_POST_LINK += $${QMAKE_MKDIR} $$COPYDEST $$SEPARATOR
        for(tmp,COPYSRC) {
          REALSRC = $$PWD/qtcreator/$$tmp
          REALDEST = $$COPYDEST/$$tmp
          win32:tmp ~= s|/+|\|
          win32:REALSRC ~= s|/+|\|
          win32:REALDEST ~= s|/+|\|
          QMAKE_POST_LINK += $${QMAKE_COPY_DIR} $${REALSRC} $${REALDEST} $$SEPARATOR
        }
    }
}

linux-* {
    keymaps.files           += $$PWD/qtcreator/schemes/MS_Visual_C++.kms
    keymaps.files           += $$PWD/qtcreator/schemes/Xcode.kms
    keymaps.path             = /share/qtcreator/schemes

    gdbsupport.files        += $$PWD/qtcreator/gdbmacros/LICENSE.LGPL
    gdbsupport.files        += $$PWD/qtcreator/gdbmacros/LGPL_EXCEPTION.TXT
    gdbsupport.files        += $$PWD/qtcreator/gdbmacros/gdbmacros.cpp
    gdbsupport.files        += $$PWD/qtcreator/gdbmacros/gdbmacros.pro
    gdbsupport.path          = /share/qtcreator/gdbmacros

    designertemplates.files += $$PWD/qtcreator/designer/templates.xml
    designertemplates.files += $$PWD/qtcreator/designer/templates/*
    designertemplates.path   = /share/qtcreator/designer/templates

    snippets.files          += $$PWD/qtcreator/snippets/*.snp
    snippets.path            = /share/qtcreator/snippets

    projecttemplates.files  += $$PWD/qtcreator/templates/qt4project/mywidget_form.h
    projecttemplates.files  += $$PWD/qtcreator/templates/qt4project/main.cpp
    projecttemplates.files  += $$PWD/qtcreator/templates/qt4project/mywidget.cpp
    projecttemplates.files  += $$PWD/qtcreator/templates/qt4project/mywidget.h
    projecttemplates.files  += $$PWD/qtcreator/templates/qt4project/widget.ui
    projecttemplates.files  += $$PWD/qtcreator/templates/qt4project/mywidget_form.cpp
    projecttemplates.path    = /share/qtcreator/templates/qt4project

    INSTALLS += \
       keymaps \
       gdbsupport \
       designertemplates \
       snippets \
       projecttemplates

}

