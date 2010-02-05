import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;
<<<<<<< HEAD:share/qtcreator/qmldesigner/propertyeditor/Qt/Layout.qml
    caption: "layout";
=======
    caption: "Layout";
>>>>>>> QmlDesigner.propertyEditor: ids have to be lower case:share/qtcreator/qmldesigner/propertyeditor/Qt/Layout.qml
id: layout;
  enabled: anchorBackend.hasParent;

  layout: QVBoxLayout {
        topMargin: 20;
        bottomMargin: 10;
        leftMargin: 40;
        rightMargin: 20;
        spacing: 20
        QLabel {
            text: "layout"
        }
        AnchorButtons {}
  }

}
