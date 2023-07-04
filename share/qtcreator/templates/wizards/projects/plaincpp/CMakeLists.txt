cmake_minimum_required(VERSION 3.5)

project(%{ProjectName} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(%{ProjectName} %{CppFileName})

include(GNUInstallDirs)
install(TARGETS %{ProjectName}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
