add_subdirectory(cplusplus)
add_subdirectory(syntax-highlighting)
add_subdirectory(libvterm)
add_subdirectory(libptyqt)

if(WIN32)
    add_subdirectory(winpty)
endif()
