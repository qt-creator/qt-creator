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
    macrosconstants.h
    macrosplugin.cpp
    macrotextfind.cpp macrotextfind.h
    macrostr.h
    texteditormacrohandler.cpp texteditormacrohandler.h
)

qtc_add_resources(Macros "images"
  PREFIX "/macros"
  BASE "."
  FILES images/macro.png
)
