import QtQuick
@if %{UseQtQuickControls2}
import QtQuick.Controls
@endif
@if %{UseImport}
import %{ApplicationImport}
@endif

%{RootItem} {
  id: root
@if %{UseImport}
    width: Constants.width
    height: Constants.height
@else
    width: 1920
    height: 1080
@endif
}
