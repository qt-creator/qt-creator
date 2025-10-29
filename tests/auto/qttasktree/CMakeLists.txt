if(Qt6_VERSION VERSION_LESS 6.11.0)
  set(TaskTree QtTaskTree)
else()
  set(TaskTree Qt::TaskTree)
endif()

add_qtc_test(tst_qttasktree
  DEPENDS Qt::Concurrent Qt::Network TaskTree
  SOURCES tst_qttasktree.cpp
)
