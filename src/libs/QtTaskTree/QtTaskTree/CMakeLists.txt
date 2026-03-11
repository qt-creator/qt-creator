add_qtc_library(QtTaskTree
  PUBLIC_DEPENDS Qt::Concurrent Qt::Core Qt::Network
  DEPENDS Qt::CorePrivate
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/..
  SOURCES
    qbarriertask.cpp
    qbarriertask.h
    qconditional.cpp
    qconditional.h
    qnetworkreplywrappertask.cpp
    qnetworkreplywrappertask.h
    qprocesstask.cpp
    qprocesstask.h
    qtasktree.cpp
    qtasktree.h
    qtasktreerunner.cpp
    qtasktreerunner.h
    qtcpsocketwrappertask.cpp
    qtcpsocketwrappertask.h
    qthreadfunctiontask.cpp
    qthreadfunctiontask.h
    qttasktreeglobal.h
)

set(other_headers
  QBarrier
  QConditional
  QCustomTask
  QDefaultTaskAdapter
  QMappedTaskTreeRunner
  QNetworkReplyWrapper
  QParallelTaskTreeRunner
  QProcessTaskAdapter
  QProcessTaskDeleter
  QSequentialTaskTreeRunner
  QSingleTaskTreeRunner
  QStartedBarrier
  QSyncTask
  QTaskInterface
  QTaskTree
  QTaskTreeTaskAdapter
  QTcpSocketWrapper
  QThreadFunction
  QThreadFunctionBase
  QThreadFunctionTaskAdapter
  QTimeoutTaskAdapter
  QtTaskTree
)
foreach(header IN LISTS other_headers)
  qtc_add_public_header(${header})
endforeach()
