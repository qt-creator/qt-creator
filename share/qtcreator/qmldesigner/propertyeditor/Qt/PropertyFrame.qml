import Qt 4.7
import Bauhaus 1.0

WidgetFrame {
   id: propertyFrame;

    minimumWidth: 300;
    property int frameWidth: width
    property int labelWidth: 76
    
    onFrameWidthChanged: {
        if (frameWidth > 300) {
            var moreSpace = (frameWidth - 300) / 3;
            var newFixedWidth = 76 + moreSpace;
            if (newFixedWidth > 200)
                newFixedWidth = 200;
            labelWidth = newFixedWidth;
        } else {
            labelWidth = 76;
        }
    }
    
    objectName:     "propertyEditorFrame"
    styleSheetFile: "propertyEditor.css";
}
