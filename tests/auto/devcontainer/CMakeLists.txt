if(Qt6_VERSION VERSION_LESS 6.11.0)
  set(TaskTree QtTaskTree)
else()
  set(TaskTree Qt::TaskTree)
endif()

add_qtc_test(tst_devcontainer
  DEPENDS DevContainer ${TaskTree}
  SOURCES tst_devcontainer.cpp
)
