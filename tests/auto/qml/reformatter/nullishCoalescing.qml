import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    property string otherString: ""
    property string aProp: otherString ?? "N/A"
    property string bProp: otherString?.trim()
    property int unrelatedProp: 10
}
