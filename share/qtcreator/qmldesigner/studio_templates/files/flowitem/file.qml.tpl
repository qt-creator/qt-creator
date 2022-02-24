/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick 2.15
@if %{UseQtQuickControls2}
import QtQuick.Controls 2.15
@endif
@if %{UseImport}
import %{ApplicationImport}
@endif
import FlowView 1.0

FlowItem {
@if %{UseImport}
    width: Constants.width
    height: Constants.height
@else
    width: 800
    height: 600
@endif
}
