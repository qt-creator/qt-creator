# -*- coding: utf-8 -*-

@if '%{Imports}'
try:
@if '%{ImportQtCore}'
    from PySide import QtCore
@endif
@if '%{ImportQtWidgets}'
    from PySide import QtWidgets
@endif
@if '%{ImportQtQuick}'
    from PySide import QtQuick
@endif
except:
@if '%{ImportQtCore}'
    from PyQt5.QtCore import pyqtSlot as Slot
    from PyQt5 import QtCore
@endif
@if '%{ImportQtWidgets}'
    from PyQt5 import QtWidgets
@endif
@if '%{ImportQtQuick}'
    from PyQt5 import QtQuick
@endif


@endif
@if '%{Base}'
class %{Class}(%{Base}):
@else
class %{Class}:
@endif
    def __init__(self):
@if '%{Base}' === 'QWidget'
        QtWidgets.QWidget.__init__(self)
@endif
@if '%{Base}' === 'QMainWindow'
        QtWidgets.QMainWindow.__init__(self)
@if '%{Base}' === 'QQuickItem'
        QtQuick.QQuickItem.__init__(self)
@endif
        pass

