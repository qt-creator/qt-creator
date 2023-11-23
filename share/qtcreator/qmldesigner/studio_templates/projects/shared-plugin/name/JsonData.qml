import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Studio.Utils 1.0

JsonBackend {
    property string name: "someName"
    property int number: 1
    source: Qt.resolvedUrl("data.json")
}
