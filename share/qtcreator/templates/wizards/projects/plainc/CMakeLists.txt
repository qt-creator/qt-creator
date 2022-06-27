cmake_minimum_required(VERSION 3.5)

project(%{ProjectName} LANGUAGES C)

add_executable(%{ProjectName} %{CFileName})

install(TARGETS %{ProjectName}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
