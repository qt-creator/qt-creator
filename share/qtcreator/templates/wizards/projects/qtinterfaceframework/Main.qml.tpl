import QtQuick
import QtQuick.Controls
import QtQuick.Window
import %{ProjectNameCap}.%{ProjectNameCap}Module

Window {
    id: root

    @if %{Zoned}
    width: 400
    @else
    width: 300
    @endif
    height: 360
    title: qsTr("%{Feature}")
    visible: true
    @if %{SampleCode}

    function getFgBits() {
        let colorBits = 0;
        colorBits |= intensityFg.checked ? %{ProjectNameCap}Module.Intensity : 0;
        colorBits |= redFg.checked ? %{ProjectNameCap}Module.Red : 0;
        colorBits |= greenFg.checked ? %{ProjectNameCap}Module.Green : 0;
        colorBits |= blueFg.checked ? %{ProjectNameCap}Module.Blue : 0;
        return colorBits;
    }
    @if %{Zoned}

    function getBgBits() {
        let colorBits = 0;
        colorBits |= intensityBg.checked ? %{ProjectNameCap}Module.Intensity : 0;
        colorBits |= redBg.checked ? %{ProjectNameCap}Module.Red : 0;
        colorBits |= greenBg.checked ? %{ProjectNameCap}Module.Green : 0;
        colorBits |= blueBg.checked ? %{ProjectNameCap}Module.Blue : 0;
        return colorBits;
    }
    @endif
    @endif

    %{Feature}Ui {
        id: my%{Feature}
        @if %{SampleCode}
        @if %{Zoned}

        function handleForeground(colorData) {
            colorRect.color = colorData.htmlCode;
            fgText.text = "Foreground\\n" + colorData.htmlCode;
        }

        function handleBackground(colorData) {
            colorRect.border.color = colorData.htmlCode;
            bgText.text = "Background\\n" + colorData.htmlCode;
        }

        onIsInitializedChanged: {
            my%{Feature}.zoneAt.Foreground?.colorProcessed.connect(handleForeground);
            my%{Feature}.zoneAt.Background?.colorProcessed.connect(handleBackground);
        }
        @else

        onColorProcessed: function(colorData) {
            colorRect.color = colorData.htmlCode;
            fgText.text = "Foreground\\n" + colorData.htmlCode;
        }
        @endif
        @endif
    }
    @if %{SampleCode}

    Grid {
        padding: 10
        spacing: 6
        @if %{Zoned}
        columns: 3
        @else
        columns: 2
        @endif

        Rectangle {
            id: colorRect
            width: 120
            height: 80
            color: "#000000"
            @if %{Zoned}
            border {
                width: 20
                color: "#000000"
            }
            @endif
        }

        Text {
            id: fgText
            text: "Foreground\\n#000000"
        }
        @if %{Zoned}

        Text {
            id: bgText
            text: "Background\\n#000000"
        }
        @endif

        Text {
            text: "Colors Processed"
        }

        Text {
            @if %{Zoned}
            text: my%{Feature}.zoneAt.Foreground?.processedCount ?? ""
            @else
            text: my%{Feature}.processedCount
            @endif
        }
        @if %{Zoned}

        Text {
            text: my%{Feature}.zoneAt.Background?.processedCount ?? ""
        }
        @endif

        Text {
            text: "EGA Intensity Bit"
        }

        CheckBox {
            id: intensityFg
        }
        @if %{Zoned}

        CheckBox {
            id: intensityBg
        }
        @endif

        Text {
            text: "EGA Red Bit"
        }

        CheckBox {
            id: redFg
        }
        @if %{Zoned}

        CheckBox {
            id: redBg
        }
        @endif

        Text {
            text: "EGA Green Bit"
        }

        CheckBox {
            id: greenFg
        }
        @if %{Zoned}

        CheckBox {
            id: greenBg
        }
        @endif

        Text {
            text: "EGA Blue Bit"
        }

        CheckBox {
            id: blueFg
        }
        @if %{Zoned}

        CheckBox {
            id: blueBg
        }
        @endif

        Text {
            text: "Gamma"
        }

        TextInput {
            width: 40
            @if %{Zoned}
            text: my%{Feature}.zoneAt.Foreground?.gamma ?? ""

            onEditingFinished: my%{Feature}.zoneAt.Foreground.gamma = text
            @else
            text: my%{Feature}.gamma

            onEditingFinished: my%{Feature}.gamma = text
            @endif

            Rectangle {
                anchors.fill: parent
                anchors.margins: -4
                border.color: "black"
                color: "transparent"
            }
        }
        @if %{Zoned}

        TextInput {
            width: 40
            text: my%{Feature}.zoneAt.Background?.gamma ?? ""

            onEditingFinished: my%{Feature}.zoneAt.Background.gamma = text

            Rectangle {
                anchors.fill: parent
                anchors.margins: -4
                border.color: "black"
                color: "transparent"
            }
        }
        @endif

        Text {
            text: "Process Colors"
        }

        Button {
            text: "Send Foreground"

            @if %{Zoned}
            onClicked: my%{Feature}.zoneAt.Foreground?.processColor(getFgBits())
            @else
            onClicked: my%{Feature}.processColor(getFgBits())
            @endif
        }
        @if %{Zoned}

        Button {
            text: "Send Background"

            onClicked: my%{Feature}.zoneAt.Background?.processColor(getBgBits())
        }
        @endif
    }
    @endif
}
