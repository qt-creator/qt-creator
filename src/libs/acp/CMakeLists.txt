add_qtc_library(AcpLib
  PUBLIC_DEPENDS Qt::Core Utils
  SOURCES
    acp.cpp
    acp.h
    acpregistry.cpp
    acpregistry.h
    acp_global.h
)

if (MSVC)
    # Prevent fatal error C1128 on large generated header
    target_compile_options(AcpLib PRIVATE /bigobj)
endif()
if (MINGW)
    target_compile_options(AcpLib PRIVATE "-Wa,-mbig-obj")
endif()
