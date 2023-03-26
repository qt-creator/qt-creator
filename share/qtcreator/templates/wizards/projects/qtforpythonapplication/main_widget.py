# This Python file uses the following encoding: utf-8
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

# Important:
# You need to run the following command to generate the ui_form.py file
#     pyside6-uic form.ui -o ui_form.py, or
#     pyside2-uic form.ui -o ui_form.py
from ui_form import Ui_%{Class}

@if '%{BaseCB}'
class %{Class}(%{BaseCB}):
@else
class %{Class}:
@endif
    def __init__(self, parent=None):
        super().__init__(parent)
        self.ui = Ui_%{Class}()
        self.ui.setupUi(self)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    widget = %{Class}()
    widget.show()
@if '%{PySideVersion}' === 'PySide6'
    sys.exit(app.exec())
@else
    sys.exit(app.exec_())
@endif
