# This Python file uses the following encoding: utf-8
import sys
@if '%{BaseCB}' === 'QWidget'
from PySide2.QtWidgets import QApplication, QWidget
@endif
@if '%{BaseCB}' === 'QMainWindow'
from PySide2.QtWidgets import QApplication, QMainWindow
@endif
@if '%{BaseCB}' === ''
from PySide2.QtWidgets import QApplication
@endif


@if '%{BaseCB}'
class %{Class}(%{BaseCB}):
@else
class %{Class}:
@endif
    def __init__(self):
@if '%{BaseCB}' === 'QWidget'
        QWidget.__init__(self)
@endif
@if '%{BaseCB}' === 'QMainWindow'
        QMainWindow.__init__(self)
@endif
@if '%{BaseCB}' === ''
        pass  # call __init__(self) of the custom base class here
@endif


if __name__ == "__main__":
    app = QApplication([])
    window = %{Class}()
@if '%{BaseCB}' === ''
    # window.show()
@else
    window.show()
@endif
    sys.exit(app.exec_())
