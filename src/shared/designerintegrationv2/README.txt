This is a new Designer integration, started on 28.02.2008.

The reason for it is the introduction of layout caching
in Qt 4.4, which unearthed a lot of mainwindow-size related
bugs in Designer and all integrations.

The goal of it is to have a closed layout chain from
integration top level to form window.

Friedemann
