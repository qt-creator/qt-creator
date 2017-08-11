import QtQml 2.0

QtObject {
    property int dings: 12 + 12 + 12
    property string no: "nonono"
    objectName: { return no + Math.random(); }
}
