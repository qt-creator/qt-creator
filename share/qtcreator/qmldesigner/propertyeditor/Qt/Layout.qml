import Qt 4.6

GroupBox {
    finished: finishedNotify;
    caption: "Layout";
id: Layout;
  minimumHeight: 280;
  enabled: anchorBackend.hasParent;

  layout: QVBoxLayout {
        topMargin: 20;
        bottomMargin: 10;
        leftMargin: 40;
        rightMargin: 20;
        spacing: 20
        QLabel {
            text: "Layout"
        }
	AnchorBox {}
  }

}
