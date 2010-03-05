import Qt 4.6

Image {
    id: screen
    property var selectedFile
    signal openFile
    source: "gradient.png"

    width: 1045
    height: 680

    Image {
          source: "welcome-card.png"

          anchors.verticalCenter: parent.verticalCenter
          anchors.horizontalCenter: parent.horizontalCenter
          transformOrigin: "Center"
          smooth: true
          scale: 0

          //Animation
          scale: SequentialAnimation {
            running: true
            NumberAnimation {
                  to: 1
                  duration: 400
                  easing: "easeOutCirc"
            }
          }
	    Text {
		    text: "Recent files"
		    style: "Sunken"
		    color: "white"
		    font.pointSize: 14
		    x: 70
		    y: 160
	    }

	    Text {
		    text: "New file"
		    style: "Sunken"
		    color: "white"
		    font.pointSize: 14
		    x: 380
		    y: 160
		    opacity: 0.8
	    }

          Item {
                id: leftSide
                width: 300
                anchors.top: parent.top
                anchors.bottom: parent.bottom

                ListView {
                    id: recentFilesListView
                    width: 280
                    height: 320
                    x: 60
                    y: 200
                    overShoot: false

                    model: recentFiles
                    delegate: fileDelegate
                }
          }

          Item {
                id: rightSide
                x: 300
                width: 300
                anchors.top: parent.top
                anchors.bottom: parent.bottom



                ListView {
                    id: templatesListView
                    width: 280
                    height: 320
                    x: 80
                    y: 200
                    overShoot: false

                    model: templatesList
                    delegate: fileDelegate
                }
          }
          Button {
                id: chooseButton
                label: " Choose"

                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 13
                anchors.rightMargin: 40
          }
    }

    Component {
          id: fileDelegate
          Item {
                width: parent.width
                height: fileDelegateText.height

                Text {
                    id: fileDelegateText
                    anchors.left: parent.left
                    color: "white"
                    text: name
                }

                MouseArea {
                    anchors.top: fileDelegateText.top
		    anchors.left: fileDelegateText.left
                    anchors.right: fileDelegateText.right
		    anchors.bottom: fileDelegateText.bottom
                    onClicked: {
                          screen.selectedFile = fileName;
                          screen.openFile();
                    }
                }
          }
    }
/*
    ListModel {
          id: recentFiles
          ListElement {
                fileName: "file1.qml"
          }
          ListElement {
                fileName: "file2.qml"
          }
          ListElement {
                fileName: "file3.qml"
          }
    }
*/

    ListModel {
          id: templatesList
          ListElement {
                fileName: ":/qmldesigner/templates/General/Empty Fx"
                name: "Fx Rectangle (640x480)"
          }
    }
}
