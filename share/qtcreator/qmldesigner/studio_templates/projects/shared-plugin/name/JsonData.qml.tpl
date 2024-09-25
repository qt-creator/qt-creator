import QtQuick
import QtQuick.Controls
import QtQuick.Studio.Utils

JsonBackend {
    property string name: "someName"
    property int number: 1
    source: Qt.resolvedUrl("data.json")
}
