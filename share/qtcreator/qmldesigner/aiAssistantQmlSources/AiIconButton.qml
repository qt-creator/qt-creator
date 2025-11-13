// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import StudioTheme as StudioTheme
import HelperWidgets as HelperWidgets

HelperWidgets.AbstractButton {
    id: control

    style: StudioTheme.Values.viewBarButtonStyle
    backgroundVisible: control.enabled && (control.hover || control.press)
}
