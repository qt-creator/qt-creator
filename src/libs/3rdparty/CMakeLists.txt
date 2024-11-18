add_subdirectory(cplusplus)
add_subdirectory(syntax-highlighting)
add_subdirectory(libvterm)
add_subdirectory(libptyqt)
add_subdirectory(qtkeychain)
add_subdirectory(lua)
add_subdirectory(sol2)
# Do not enable it yet, as the necessary changes are only introduced with the next patch.
#add_subdirectory(karchive)

if(WIN32)
    add_subdirectory(winpty)
endif()

if (BUILD_DESIGNSTUDIO)
    add_subdirectory(qrcodegen)
endif()
