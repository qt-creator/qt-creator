TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    cplusplus \
    debugger \
    diff \
    extensionsystem \
    externaltool \
    environment \
    generichighlighter \
    profilewriter \
    treeviewfind \
    ioutils \
    qtcprocess \
    utils \
    utils_stringutils \
    filesearch \
    valgrind

lessThan(QT_MAJOR_VERSION, 5) {
  contains(QT_CONFIG, declarative) {
    SUBDIRS += qml
  }
} else {
  qtHaveModule(declarative) {
    SUBDIRS += qml qmlprofiler
  }
}
