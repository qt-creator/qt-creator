Extra patches to LLVM/Clang 5.0
===============================

The patches in this directory are applied to LLVM/Clang with:

    $ cd $LLVM_SOURCE_DIR
    $ git apply --whitespace=fix $QT_CREATOR_SOURCE/dist/clang/patches/*.patch

Backported changes
------------------

##### D35355_Fix-templated-type-alias-completion-when-using-global-completion-cache.patch

* <https://reviews.llvm.org/D35355>

Fixes completion involving templated type alias.

##### D37435_Dont-show-deleted-function-constructor-candidates-for-code-completion.patch

* <https://reviews.llvm.org/D37435>
* <https://bugs.llvm.org/show_bug.cgi?id=34402>

Fixes completion involving implicitly deleted copy constructors.

##### rL310905_Avoid-PointerIntPair-of-constexpr-EvalInfo-structs.patch

* <https://reviews.llvm.org/rL310905>
* <https://bugs.llvm.org/show_bug.cgi?id=32018>

Fixes build/miscompilation of LLVM/Clang with MinGW, which enables generation
of profile-guided-optimization builds for Windows.

Additional changes
------------------

##### QTCREATORBUG-15449_Fix-files-lock-on-Windows.patch

* <https://reviews.llvm.org/D35200>
* <https://bugreports.qt.io/browse/QTCREATORBUG-15449>

Significantly reduces problems when saving a header file on Windows.

##### QTCREATORBUG-15157_Link-with-clazy.patch

* <https://bugreports.qt.io/browse/QTCREATORBUG-15157>

Introduces the flag CLANG_ENABLE_CLAZY to link libclang with Clazy and forces
link for Clazy checks and plugin registry entry.

