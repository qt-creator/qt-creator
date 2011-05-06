import Qt 4.7
import Bauhaus 1.0

WidgetFrame {
   id: propertyFrame;

    minimumWidth: 300;
    property int frameWidth: width
    property int labelWidth: 96
    
    onFrameWidthChanged: {
        if (frameWidth > 300) {
            var moreSpace = (frameWidth - 300) / 3;
            var newFixedWidth = 96 + moreSpace;
            if (newFixedWidth > 200)
                newFixedWidth = 200;
            labelWidth = newFixedWidth;
        } else {
            labelWidth = 96;
        }
    }
    
    objectName:     "propertyEditorFrame"
    styleSheetFile: "propertyEditor.css";
}
