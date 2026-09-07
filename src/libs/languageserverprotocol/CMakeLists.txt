add_qtc_library(LanguageServerProtocol
  PUBLIC_DEPENDS Qt::Core Utils
  SOURCES
    languageserverprotocol_global.h
    languageserverprotocoltr.h
    lspbasemessage.cpp
    lspbasemessage.h
    lspjsonrpc.cpp
    lspjsonrpc.h
    lspmessages.h
    lsputils.cpp
    lsputils.h
    lsptypes.cpp
    lsptypes.h
)

if (MSVC)
    # Prevent fatal error C1128 on large generated header
    target_compile_options(LanguageServerProtocol PRIVATE /bigobj)
endif()
if (MINGW)
    target_compile_options(LanguageServerProtocol PRIVATE "-Wa,-mbig-obj")
endif()
