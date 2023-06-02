add_qtc_library(Tasking OBJECT
# Never add dependencies to non-Qt libraries for this library
  DEPENDS Qt::Core
  PUBLIC_DEFINES TASKING_LIBRARY
  SOURCES
    barrier.cpp barrier.h
    tasking_global.h
    tasktree.cpp tasktree.h
)
