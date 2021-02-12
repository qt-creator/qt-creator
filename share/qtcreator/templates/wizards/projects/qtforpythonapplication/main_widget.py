# This Python file uses the following encoding: utf-8
import sys
import os


@if '%{BaseCB}' === 'QWidget'
from %{PySideVersion}.QtWidgets import QApplication, QWidget
@endif
@if '%{BaseCB}' === 'QMainWindow'
from %{PySideVersion}.QtWidgets import QApplication, QMainWindow
@endif
@if '%{BaseCB}' === 'QDialog'
from %{PySideVersion}.QtWidgets import QApplication, QDialog
@endif
from %{PySideVersion}.QtCore import QFile
from %{PySideVersion}.QtUiTools import QUiLoader


@if '%{BaseCB}'
class %{Class}(%{BaseCB}):
@else
class %{Class}:
@endif
    def __init__(self):
        super(%{Class}, self).__init__()
        self.load_ui()

    def load_ui(self):
        loader = QUiLoader()
        path = os.path.join(os.path.dirname(__file__), "form.ui")
        ui_file = QFile(path)
        ui_file.open(QFile.ReadOnly)
        loader.load(ui_file, self)
        ui_file.close()

if __name__ == "__main__":
    app = QApplication([])
    widget = %{Class}()
    widget.show()
    sys.exit(app.exec_())
