import QtQuick %{QtQuickVersion}
import QtQuick.Controls %{QtQuickVersion}
@if !%{IsQt6Project}
import QtQuick.Window %{QtQuickVersion}
@endif
import %{ImportModuleName} %{ImportModuleVersion}

Window {
    width: Constants.width
    height: Constants.height

    visible: true

    SwipeView {
        id: swipeView
        anchors.top: tabBar.bottom
        anchors.right: parent.right
          anchors.bottom: parent.bottom
          anchors.left: parent.left
          currentIndex: tabBar.currentIndex

          Screen01 {
          }

          Screen02 {
          }
      }

      TabBar {
          anchors.left: parent.left
          anchors.right: parent.right

          id: tabBar
          currentIndex: swipeView.currentIndex

          TabButton {
              text: qsTr("Page 1")
          }
          TabButton {
              text: qsTr("Page 2")
          }
      }
}
