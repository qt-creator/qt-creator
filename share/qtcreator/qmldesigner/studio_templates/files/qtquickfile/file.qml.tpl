import QtQuick 2.15
@if %{UseQtQuickControls2}
import QtQuick.Controls 2.15
@endif
@if %{UseImport}
import %{ApplicationImport}
@endif

%{RootItem} {
@if %{UseImport}
    width: Constants.width
    height: Constants.height
@else
    width: 1920
    height: 1080
@endif
}
