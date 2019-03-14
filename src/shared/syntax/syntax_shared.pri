# KSyntaxHighlighting uses a special folder structure (may contain target arch triple for the lib),
# so expect lib folder to be specified by the user - generate include path based on this
isEmpty(KSYNTAXHIGHLIGHTING_LIB_DIR): KSYNTAXHIGHLIGHTING_LIB_DIR=$$(KSYNTAXHIGHLIGHTING_LIB_DIR)
isEmpty(KSYNTAXHIGHLIGHTING_INCLUDE_DIR): KSYNTAXHIGHLIGHTING_INCLUDE_DIR=$$(KSYNTAXHIGHLIGHTING_INCLUDE_DIR)

!isEmpty(KSYNTAXHIGHLIGHTING_LIB_DIR) {
    !exists($$KSYNTAXHIGHLIGHTING_LIB_DIR) {
        unset(KSYNTAXHIGHLIGHTING_LIB_DIR)
        unset(KSYNTAXHIGHLIGHTING_INCLUDE_DIR)
    } else {
        isEmpty(KSYNTAXHIGHLIGHTING_INCLUDE_DIR) {
            !linux: KSYNTAXHIGHLIGHTING_INCLUDE_DIR=$$absolute_path('../include/KF5/KSyntaxHighlighting/', $$KSYNTAXHIGHLIGHTING_LIB_DIR)
            else:  KSYNTAXHIGHLIGHTING_INCLUDE_DIR=$$absolute_path('../../include/KF5/KSyntaxHighlighting/', $$KSYNTAXHIGHLIGHTING_LIB_DIR)
        }

        !exists($$KSYNTAXHIGHLIGHTING_INCLUDE_DIR) {
            unset(KSYNTAXHIGHLIGHTING_INCLUDE_DIR)
            unset(KSYNTAXHIGHLIGHTING_LIB_DIR)
        }
    }
}
