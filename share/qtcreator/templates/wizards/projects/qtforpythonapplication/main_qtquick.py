# This Python file uses the following encoding: utf-8
import sys
from pathlib import Path

from %{PySideVersion}.QtGui import QGuiApplication
from %{PySideVersion}.QtQml import QQmlApplicationEngine


if __name__ == "__main__":
    app = QGuiApplication(sys.argv)
    engine = QQmlApplicationEngine()
@if '%{PySideVersion}' === 'PySide6'
    engine.addImportPath(Path(__file__).parent)
    engine.loadFromModule("%{ProjectName}", "%{JS: Util.baseName('%{QmlFileName}')}")
@else
    qml_file = Path(__file__).resolve().parent / "%{ProjectName}" / "%{QmlFileName}"
    engine.load(str(qml_file))
@endif
    if not engine.rootObjects():
        sys.exit(-1)
@if '%{PySideVersion}' === 'PySide6'
    sys.exit(app.exec())
@else
    sys.exit(app.exec_())
@endif
