import QtQuick %{QtQuickVersion}
@if !%{IsQt6Project}
import QtQuick.Window %{QtQuickVersion}
@endif
import %{ApplicationImport}

Window {
    width: Constants.width
    height: Constants.height

    visible: true

    Screen01 {
        width: parent.width
        height: parent.height
    }
}
