Extra patches to LLVM/Clang 5.0
===============================

The patches in this directory are applied to LLVM/Clang with:

    $ cd $LLVM_SOURCE_DIR
    $ git apply --whitespace=fix $QT_CREATOR_SOURCE/dist/clang/patches/*.patch

Backported changes
------------------

##### 110_D41016_Fix-crash-in-unused-lambda-capture-warning-for-VLAs.patch

* <https://reviews.llvm.org/D41016>

[Sema] Fix crash in unused-lambda-capture warning for VLAs

##### 010_D35355_Fix-templated-type-alias-completion-when-using-global-completion-cache.patch

* <https://reviews.llvm.org/D35355>

Fixes completion involving templated type alias.

##### 120_D41688_Fix-crash-on-code-completion-in-comment-in-included-file.patch

* <https://reviews.llvm.org/D41688>

[Lex]  Fix crash on code completion in comment in included file.

##### 100_D40841_Fix-a-crash-on-C++17-AST-for-non-trivial-construction-into-a-trivial-brace-initialize.patch

* <https://reviews.llvm.org/D40841>

[analyzer] Fix a crash on C++17 AST for non-trivial construction into a trivial brace initializer.

##### 090_D40746_Correctly-handle-line-table-entries-without-filenames-during-AST-serialization.patch

* <https://reviews.llvm.org/D40746>

Correctly handle line table entries without filenames during AST serialization
Fixes crash during a reparse.

##### 050_D40027_Fix-cursors-for-in-class-initializer-of-field-declarations.patch

* <https://reviews.llvm.org/D40027>
* <https://bugs.llvm.org/show_bug.cgi?id=33745>

[libclang] Fix cursors for in-class initializer of field declarations
Fixes AST access to initializers of class members. Affects mostly semantic highlighting and highlighting of local uses.

##### 070_D40561_Fix-cursors-for-functions-with-trailing-return-type.patch

* <https://reviews.llvm.org/D40561>

[libclang] Fix cursors for functions with trailing return type

##### 060_D40072_Support-querying-whether-a-declaration-is-invalid.patch

* <https://reviews.llvm.org/D40072>

[libclang] Add support for checking abstractness of records
Would need https://codereview.qt-project.org/#/c/211497/ on Qt Creator side.

##### 040_D39957_Honor-TerseOutput-for-constructors.patch

* <https://reviews.llvm.org/D39957>

[DeclPrinter] Honor TerseOutput for constructors
Avoids printing member initialization list and body for constructor.

##### 080_D40643_Add-function-to-get-the-buffer-for-a-file.patch

* <https://reviews.llvm.org/D40643>
* <https://reviews.llvm.org/rL319881>

[libclang] Add function to get the buffer for a file
Together with https://codereview.qt-project.org/#/c/212972/ fixes highlighting

##### 030_D38615_Only-mark-CXCursors-for-explicit-attributes-with-a-type.patch

* <https://reviews.llvm.org/D38615>

[libclang] Only mark CXCursors for explicit attributes with a type
Some classes have totally broken highlighting (like classes inside texteditor.cpp)

##### 170_D40013_DeclPrinter-Allow-printing-fully-qualified-name.patch
##### 180_D39903_libclang-Allow-pretty-printing-declarations.patch

* <https://reviews.llvm.org/D40013>
* <https://reviews.llvm.org/D39903>

[DeclPrinter] Allow printing fully qualified name of function declaration
[libclang] Allow pretty printing declarations

Improves pretty printing for tooltips.

##### 220_Support-std-has_unique_object_represesentations.patch

* https://reviews.llvm.org/D39064 mplement __has_unique_object_representations
* https://reviews.llvm.org/D39347 Fix __has_unique_object_representations implementation
* (without review, git sha1 133cba2f9263f63f44b6b086a500f374bff13eee) Fix ICE when __has_unqiue_object_representations called with invalid decl
* (without review, git cb61fc53dc997bca3bee98d898d3406d0acb221c) Revert unintended hunk from ICE-Change
* https://reviews.llvm.org/D42863 Make __has_unique_object_representations reject empty union types.

Backport patches implementing std::has_unique_object_representations for
parsing type_traits header of stdlibc++ 7.


##### 230_D40673_Add-Float128-as-alias-to-__float128.patch

* https://reviews.llvm.org/D40673

Fixes parsing stdlib.h with -DGNU_SOURCE for GCC 7.2 (and maybe others).

Additional changes
------------------

##### 160_QTCREATORBUG-15449_Fix-files-lock-on-Windows.patch

* <https://reviews.llvm.org/D35200>
* <https://bugreports.qt.io/browse/QTCREATORBUG-15449>

Significantly reduces problems when saving a header file on Windows.

##### 150_QTCREATORBUG-15157_Link-with-clazy_llvm.patch
##### 130_QTCREATORBUG-15157_Link-with-clazy_clang.patch
##### 140_QTCREATORBUG-15157_Link-with-clazy_extra.patch

* <https://bugreports.qt.io/browse/QTCREATORBUG-15157>

Builds Clazy as an LLVM part and forces link for Clazy plugin registry entry.

##### 200_D36390_Fix-overloaded-static-functions-in-SemaCodeComplete.patch

* <https://reviews.llvm.org/D36390>
* <https://bugs.llvm.org/show_bug.cgi?id=33904>

Fix overloaded static functions in SemaCodeComplete

Happens when static function is accessed via the class variable.
That leads to incorrect overloads number because the variable is considered as the first argument.

##### 210_D43453_Fix-overloaded-static-functions-for-templates.patch

* <https://reviews.llvm.org/D43453>

Fix overloaded static functions for templates

Apply almost the same fix as D36390 but for templates
