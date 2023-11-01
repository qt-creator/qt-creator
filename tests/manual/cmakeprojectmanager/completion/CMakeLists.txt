# Only parameters for the function should be mentioned,
# In the case for cmake_minimum_required should be
# VERSION and FATAL_ERROR
cmake_minimum_required(VERSION 3.15)

# Mouse hover over "project" should display a help popup
project(completion LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# F2 on the URL should open the url in the web browser
# https://doc.qt.io/qt-6/cmake-get-started.html

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# here code completion for "find_package(Qt|" should popup
# Qt, Qt3, Qt4 and Qt6
#find_package(Qt

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# here code completion "find_package(std|" should popup stdvector
#find_package(std

qt_standard_project_setup()

set(SOURCE_FILES
  mainwindow.ui
  mainwindow.cpp mainwindow.h
  main.cpp

  # here code completion for "main|" should popup the source files
  #main
)

# here code completion for "if (SOU|" should popup SOURCE_FILES
#if (SOU

if (SOURCE_FILES)
  # here code completion for "print_v|" should popup "print_variables"
  #print_v

  # here code completion for "include(CMakePrint|" should popup "CMakePrintHelpers"
  #include(CMakePrint

  #
  # F2 on CMakePrintHelpers would open the CMake module
  #    on cmake_print_variables would open the cursor at the function
  #    on SOURCE_FILES would jump above to the set(SOURCE_FILES
  #
  include(CMakePrintHelpers)
  cmake_print_variables(SOURCE_FILES)
endif()

# here code completion for "if (ENV{|" should popup the CMake supported environment variables
# also deleting { and typing again { should trigger the menu
#if (ENV{

# hover over CXX should display the help for the CXX environment variable
# and F1 should open the help page
message(STATUS $ENV{CXX})

qt_add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# here code completino on "target_link_libraries(comp|" should poupup "completion"
# with a hammer icon, which suggests a project target
#target_link_libraries(comp

# here code completion after "target_link_libraries(completion PRIVATE Qt6::Wid|"
# should complete with "Qt6::Widgets" having a grayish hammer icon

#target_link_libraries(completion PRIVATE Qt6::Wid


# F2 on "completion" would jump above to qt_add_executable
target_link_libraries(completion PRIVATE Qt6::Widgets)

# F2 on ${PROJECT_NAME} would jump above to qt_add_executable
set_target_properties(${PROJECT_NAME} PROPERTIES
  # F1 on WIN32_EXECUTABLE should open the help
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON

  # here completion for "WIN32|" should popup WIN32_EXECUTABLE
  #WIN32
)

include(JustACacheVariable)
# here code completion for "if (JUST|" should complete with if (JUST_A_CACHE_VARIABLE
# as a tooltip containing the description
#if (JUST
