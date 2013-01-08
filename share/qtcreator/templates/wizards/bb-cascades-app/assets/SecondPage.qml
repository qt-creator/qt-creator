import bb.cascades 1.0

Page {
    id: seconPage
    paneProperties: NavigationPaneProperties {
        backButton: ActionItem {
            onTriggered: {
                navigationPane.pop()
            }
        }
    }
    Container {
        layout: DockLayout {}
        Label {
            text: qsTr("Hello Cascades!")
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Center
            textStyle {
                base: SystemDefaults.TextStyles.TitleText
            }
        }
    }
}

