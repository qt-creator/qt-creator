add_qtc_plugin(Macros
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    actionmacrohandler.cpp actionmacrohandler.h
    findmacrohandler.cpp findmacrohandler.h
    imacrohandler.cpp imacrohandler.h
    macro.cpp macro.h
    macroevent.cpp macroevent.h
    macrolocatorfilter.cpp macrolocatorfilter.h
    macromanager.cpp macromanager.h
    macrooptionspage.cpp macrooptionspage.h
    macros.qrc
    macrosconstants.h
    macrosplugin.cpp macrosplugin.h
    macrotextfind.cpp macrotextfind.h
    macrostr.h
    savedialog.cpp savedialog.h
    texteditormacrohandler.cpp texteditormacrohandler.h
)
