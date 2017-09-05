# .clang-format for Qt Creator

Alongside this file you find an EXPERIMENTAL .clang-format configuration file
for the Qt Creator code base.

The current configuration is useful, but not fully in accordance with the
coding rules. There is also other undesired formatting. Running clang-format
blindly will not only improve formatting here and there, but will also
normalize/worsen code that is already considered ideally formatted. See section
"Coding rules violated by clang-format" below for more information.

If needed, clang-format can be instructed to not format code ranges. Do not
overuse this.

    // clang-format off
        void    unformatted_code  ;
    // clang-format on

For more information about clang-format, see

      <https://clang.llvm.org/docs/ClangFormat.html>

## Prerequisites

 * clang-format >= 5.0

## Set up Qt Creator for use with clang-format

### Install the configuration file

For a given source file to format, clang-format it will read the configuration
from .clang-format in the closest parent directory for the file to format.

Hence symlink/copy .clang-format from this directory to e.g. Qt Creator's top
level directory:

For Linux/macOS:

    $ cd $QTC_SOURCE
    $ ln -s dist/clangformat/.clang-format

For Windows:

    $ cd $QTC_SOURCE
    $ copy dist\clangformat\.clang-format # Do not forget to keep this updated

### Configure Qt Creator

  1. Enable the Beautifier plugin and restart to load it.

  2. Configure the plugin:
     In Menu: Tools > Options > Beautifier > Tab: Clang Format
      * Select a valid clang-format executable
      * Use predefined style: File
          * Fallback style: None

  3. Set shortcuts for convenience:
     In Menu: Tools > Options > Environment > Keyboard
      * ClangFormat / FormatFile - e.g. Alt+C, F
      * ClangFormat / FormatAtCursor - e.g. Alt+C, C
      * ClangFormat / DisableFormattingSelectedText - e.g. Alt+C, D

Due to several issues outlined below the FormatFile action might be of limited
use.

## Coding rules enforced by clang-format

This is a copy-pasted list of coding rules from
<https://doc-snapshots.qt.io/qtcreator-extending/coding-style.html> that can be
enforced with the current configuration:

* Formatting
  * Whitespace
    * Use four spaces for indentation, no tabs.
    * Always use only one blank line (to group statements together)
    * Pointers and References: For pointers or references, always use a single
      space before an asterisk (*) or an ampersand (&), but never after.
    * Operator Names and Parentheses: Do not use spaces between operator names
      and parentheses.
    * Function Names and Parentheses: Do not use spaces between function names
      and parentheses.
    * Keywords: Always use a single space after a keyword, and before a curly
      brace.
  * Braces
    * As a base rule, place the left curly brace on the same line as the start
      of the statement.
    * Exception: Function implementations and class declarations always have
      the left brace in the beginning of a line
  * Line Breaks
    * Keep lines shorter than 100 characters
    * Insert line breaks if necessary.
    * Commas go at the end of a broken line.
    * Operators start at the beginning of the new line.
  * Namespaces:
    * Put the left curly brace on the same line as the namespace keyword.
    * Do not indent declarations or definitions inside.
    * Optional, but recommended if the namespaces spans more than a few lines:
      Add a comment after the right curly brace repeating the namespace.
* Patterns and Practices
  * C++11 and C++14 Features / Lambdas:
    * Optionally, place the lambda completely on one line if it fits.
    * Place a closing parenthesis and semicolon of an enclosing function call
      on the same line as the closing brace of the lambda.

## Coding rules violated by clang-format

* Formatting / Namespaces
  * As an exception, if there is only a single class declaration inside the
    namespace, all can go on a single line.  Currently this ends up on several
    lines, which is noisy.
* Patterns and Practices / C++11 and C++14 Features / Lambdas
  * If you are using a lambda in an 'if' statement, start the lambda on a new
    line.

### Other undesired formattings

For a more complete list of observations and problems, see the "// NOTE: "
comments in ../../tests/manual/clang-format-for-qtc/test.cpp.

#### Manually aligned complex expressions in e.g. if-conditions are collapsed

We want:

    if (someFancyAndLongerExpression1
            || someFancyAndLongerExpression2
            || someFancyAndLongerExpression3) {
        return value;
    }

Current:

    if (someFancyAndLongerExpression1 || someFancyAndLongerExpression2
        || someFancyAndLongerExpression3) {
        return value;
    }

#### connect() calls do not follow the standard two-line pattern

In general, for middle-sized and longer connect() calls we want to follow this
two-line pattern.

    connect(sender, &SomeFancySender::signal,
            receiver, &SomeFancyReceiver::slot);

Current:

    connect(sender,
            &SomeFancySender::signal,
            receiver,
            &SomeFancySender::slot);

#### QTC_ASSERT and excess space after return keyword

For QTC_ASSERT in combination with the return keyword, an excess space is added:

    QTC_ASSERT(headItem, return );

