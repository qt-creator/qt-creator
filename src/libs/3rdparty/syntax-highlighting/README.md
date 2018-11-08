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
