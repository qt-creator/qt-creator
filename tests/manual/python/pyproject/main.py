#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import os
import sys

from PySide2.QtGui import QGuiApplication
from PySide2.QtQml import QQmlApplicationEngine


if __name__ == "__main__":
    app = QGuiApplication(sys.argv)

    engine = QQmlApplicationEngine()
    engine.load(os.path.join(os.path.dirname(__file__), "main.qml"))

    if not engine.rootObjects():
        sys.exit(-1)

    sys.exit(app.exec_())
