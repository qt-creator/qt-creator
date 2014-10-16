TEMPLATE = subdirs
CONFIG += ordered

#only build on same condition as clang code model plugin
isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
!isEmpty(LLVM_INSTALL_DIR) {
    SUBDIRS += plugins/clangstaticanalyzer
}

QMAKE_EXTRA_TARGETS = docs install_docs # dummy targets for consistency
