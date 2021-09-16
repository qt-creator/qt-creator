QT += network \
      qml \
      quick \
      sql \
      testlib \
      widgets

qtHaveModule(multimedia) {
    QT += multimedia
}

qtHaveModule(multimediawidgets) {
    QT += multimediawidgets
}

TARGET = qt-essential-includes
TEMPLATE = app
SOURCES += main.cpp
