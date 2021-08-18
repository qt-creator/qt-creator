# This Python file uses the following encoding: utf-8
import os
from pathlib import Path
import sys

from %{PySideVersion}.QtGui import QGuiApplication
from %{PySideVersion}.QtQml import QQmlApplicationEngine


if __name__ == "__main__":
    app = QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()
    engine.load(os.fspath(Path(__file__).resolve().parent / "%{QmlFileName}"))
    if not engine.rootObjects():
        sys.exit(-1)
    sys.exit(app.exec_())
