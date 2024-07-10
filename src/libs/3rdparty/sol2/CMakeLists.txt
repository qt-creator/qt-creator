add_library(sol2 INTERFACE)

target_include_directories(sol2 INTERFACE  
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:include>
)

install(TARGETS sol2 EXPORT QtCreator)

qtc_add_public_header(include/sol/sol.hpp)
qtc_add_public_header(include/sol/config.hpp)
qtc_add_public_header(include/sol/forward.hpp)
