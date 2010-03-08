import Qt 4.6
import Bauhaus 1.0

QWidget {
    id: expressionEditor;
    x: 6
    y: -400
    width: frame.width - 22
    height: 40
    property bool active: false
    property var backendValue;


    onActiveChanged: {
        //text = "";
        print("active")
        print(y)
        if (active) {
            textEdit.plainText = backendValue.expression
            textEdit.setFocus();
            opacity = 1;
            height = 60

        } else {
            opacity = 0;
            height = 40;
        }
    }

    opacity: 0
    onOpacityChanged: {
        if (opacity == 0)
            lower();
    }
    Behavior on opacity {
        NumberAnimation {
            duration: 100
        }
    }

    Behavior on height {
        NumberAnimation {
            duration: 100
        }
    }


    ExpressionEdit {
        id: textEdit;
        styleSheet: "QTextEdit {border-radius: 0px;}"
        documentTitle: "Expression"

        width: expressionEdit.width
        height: expressionEdit.height
        horizontalScrollBarPolicy: "Qt::ScrollBarAlwaysOff"
        verticalScrollBarPolicy: "Qt::ScrollBarAlwaysOff"

        onFocusChanged: {
            if (!focus)
                expressionEdit.active = false;
        }
		onReturnPressed: {
		    expressionEdit.backendValue.expression = textEdit.plainText;
			expressionEdit.active = false;		
		}
    }
	
    QPushButton {
        focusPolicy: "Qt::NoFocus";
        y: expressionEdit.height - 22;
        x: expressionEdit.width - 61;
        styleSheetFile: "applybutton.css";
        width: 29
        height: 19
        onClicked: {
            expressionEdit.backendValue.expression = textEdit.plainText;
            expressionEdit.active = false;
        }
    }

    QPushButton {
        focusPolicy: "Qt::NoFocus";
        y: expressionEdit.height - 22;
        x: expressionEdit.width - 32;
        styleSheetFile: "cancelbutton.css";
        width: 29
        height: 19
        onClicked: {
            expressionEdit.active = false;
        }
    }
}
