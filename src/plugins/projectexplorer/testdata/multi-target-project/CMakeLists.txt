project(multi-target-project)
add_executable(multi-target-project-app multi-target-project-main.cpp multi-target-project-shared.h)
add_library(multi-target-project-lib STATIC multi-target-project-lib.cpp multi-target-project-shared.h)
