import bb.cascades 1.0

NavigationPane {
    id: navigationPane
    Page {
        Container {
            layout: DockLayout {}
            Button {
                horizontalAlignment: HorizontalAlignment.Center
                verticalAlignment: VerticalAlignment.Center
                text: qsTr("Next")
                onClicked: {
                    var page = getSecondPage();
                    navigationPane.push(page);
                }
                property Page secondPage
                function getSecondPage() {
                    if (! secondPage) {
                        secondPage = secondPageDefinition.createObject();
                    }
                    return secondPage;
                }
                attachedObjects: [
                    ComponentDefinition {
                        id: secondPageDefinition
                        source: "SecondPage.qml"
                    }
                ]
            }
        }
    }
    onCreationCompleted: {
        console.log("NavigationPane - onCreationCompleted()");
        OrientationSupport.supportedDisplayOrientation = SupportedDisplayOrientation.All;
    }
}

