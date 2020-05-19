# This Python file uses the following encoding: utf-8
@if '%{Module}' !== '<None>'
    @if '%{ImportQtCore}' !== ''
from %{Module} import QtCore
    @endif
    @if '%{ImportQtWidgets}' !== ''
from %{Module} import QtWidgets
    @endif
    @if '%{ImportQtQuick}' !== ''
from %{Module} import QtQuick
    @endif
@endif


@if '%{Base}'
class %{Class}(%{FullBase}):
@else
class %{Class}:
@endif
    def __init__(self):
@if %{JS: [ "QObject", "QWidget", "QMainWindow", "QQuickItem" ].indexOf("%Base")} >= 0
        %{FullBase}.__init__(self)
@endif
        pass
