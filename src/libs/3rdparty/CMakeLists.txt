add_subdirectory(cplusplus)
add_subdirectory(syntax-highlighting)
add_subdirectory(libvterm)
add_subdirectory(libptyqt)
add_subdirectory(qrcodegen)
add_subdirectory(qtkeychain)
add_subdirectory(lua)
add_subdirectory(sol2)

if(WIN32)
    add_subdirectory(winpty)
endif()

add_subdirectory(xz)
add_subdirectory(bzip2)

set(ENABLE_WERROR OFF)
add_subdirectory(libarchive)
