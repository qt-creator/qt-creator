# This Python file uses the following encoding: utf-8
import sys
@if '%{BaseCB}' === 'QWidget'
from %{PySideVersion}.QtWidgets import QApplication, QWidget
@endif
@if '%{BaseCB}' === 'QMainWindow'
from %{PySideVersion}.QtWidgets import QApplication, QMainWindow
@endif
@if '%{BaseCB}' === ''
from %{PySideVersion}.QtWidgets import QApplication
@endif


@if '%{BaseCB}'
class %{Class}(%{BaseCB}):
@else
class %{Class}:
@endif
    def __init__(self, parent=None):
@if '%{BaseCB}' === 'QWidget' || '%{BaseCB}' === 'QMainWindow'
        super().__init__(parent)
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
@if '%{PySideVersion}' === 'PySide6'
    sys.exit(app.exec())
@else
    sys.exit(app.exec_())
@endif
