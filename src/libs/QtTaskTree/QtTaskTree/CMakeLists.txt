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
  QAbstractTaskTreeRunner
  QBarrier
  QThreadFunction
  QThreadFunctionBase
  QCustomTask
  QDefaultTaskAdapter
  QMappedTaskTreeRunner
  QNetworkReplyWrapper
  QParallelTaskTreeRunner
  QProcessDeleter
  QProcessTaskAdapter
  QSequentialTaskTreeRunner
  QSingleTaskTreeRunner
  QStartedBarrier
  QSyncTask
  QTaskInterface
  QTaskTree
  QTaskTreeTaskAdapter
  QTcpSocketWrapper
  QTimeoutTaskAdapter
)
foreach(header IN LISTS other_headers)
  qtc_add_public_header(${header})
endforeach()
