# This Python file uses the following encoding: utf-8
import os
from pathlib import Path
import sys

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
    def __init__(self, parent=None):
        super().__init__(parent)
        self.load_ui()

    def load_ui(self):
        loader = QUiLoader()
        path = Path(__file__).resolve().parent / "form.ui"
@if '%{PySideVersion}' === 'PySide6'
        ui_file = QFile(path)
@else
        ui_file = QFile(str(path))
@endif
        ui_file.open(QFile.ReadOnly)
        loader.load(ui_file, self)
        ui_file.close()


if __name__ == "__main__":
    app = QApplication(sys.argv)
    widget = %{Class}()
    widget.show()
@if '%{PySideVersion}' === 'PySide6'
    sys.exit(app.exec())
@else
    sys.exit(app.exec_())
@endif
