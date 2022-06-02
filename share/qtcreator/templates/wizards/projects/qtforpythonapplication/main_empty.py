# This Python file uses the following encoding: utf-8
import sys
from %{PySideVersion}.QtWidgets import QApplication


if __name__ == "__main__":
    app = QApplication(sys.argv)
    # ...
@if '%{PySideVersion}' === 'PySide6'
    sys.exit(app.exec())
@else
    sys.exit(app.exec_())
@endif
