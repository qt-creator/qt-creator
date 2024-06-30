add_qtc_library(Tasking OBJECT
# Never add dependencies to non-Qt libraries for this library
  DEPENDS Qt::Concurrent Qt::Core Qt::Network
  SOURCES
    barrier.cpp
    barrier.h
    concurrentcall.h
    conditional.cpp
    conditional.h
    networkquery.cpp
    networkquery.h
    qprocesstask.cpp
    qprocesstask.h
    tasking_global.h
    tasktree.cpp
    tasktree.h
    tasktreerunner.cpp
    tasktreerunner.h
  EXPLICIT_MOC
    networkquery.h
)
