# Syntax Highlighting

Syntax highlighting engine for Kate syntax definitions

## Introduction

This is a stand-alone implementation of the Kate syntax highlighting engine.
It's meant as a building block for text editors as well as for simple highlighted
text rendering (e.g. as HTML), supporting both integration with a custom editor
as well as a ready-to-use QSyntaxHighlighter sub-class.

## Syntax Definition Files

This library uses Kate syntax definition files for the actual highlighting,
the file format is documented [here](https://docs.kde.org/stable5/en/applications/katepart/highlight.html).

More than 250 syntax definition files are included, additional ones are
picked up from the file system if present, so you can easily extend this
by application-specific syntax definitions for example.

## Out of scope

To not turn this into yet another text editor, the following things are considered
out of scope:

* code folding, beyond providing folding range information
* auto completion
* spell checking
* user interface for configuration
* management of text buffers or documents

If you need any of this, check out [KTextEditor](https://api.kde.org/frameworks/ktexteditor/html/).

## Adding unit tests for a syntax definition

* add an input file into the autotests/input/ folder, lets call it test.<language-extension>

* if the file extension is not sufficient to trigger the right syntax definition, you can add an
  second file testname.<language-extension>.syntax that contains the syntax definition name
  to enforce the use of the right extension

* do "make && make test"

* inspect the outputs found in your binary directory autotests/folding.out, autotests/html.output and autotests/output

* if ok, run in the binary folder "./autotests/update-reference-data.sh" to copy the results to the right location

* add the result references after the copying to the git

