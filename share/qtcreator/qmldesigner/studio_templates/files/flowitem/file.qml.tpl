import QtQuick 2.15
@if %{UseQtQuickControls2}
import QtQuick.Controls 2.15
@endif
@if %{UseImport}
import %{ApplicationImport}
@endif
import FlowView 1.0

FlowItem {
    width: Constants.width
    height: Constants.height
}
