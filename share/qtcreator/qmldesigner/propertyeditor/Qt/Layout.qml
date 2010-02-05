import Qt 4.6
import Bauhaus 1.0

GroupBox {
    finished: finishedNotify;

    caption: "layout";

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
