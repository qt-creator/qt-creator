import Qt 4.6

Item {
    width: 640;
    height: 480;
    Rectangle {
        width: 295;
        height: 264;
        color: "#009920";
        x: 22;
        y: 31;
        id: rectangle_1;
        Rectangle {
            width: 299;
            color: "#009920";
            x: 109;
            height: 334;
            y: 42;
            id: rectangle_3;
            Rectangle {
                color: "#009920";
                height: 209;
                width: 288;
                x: 98;
                y: 152;
                id: rectangle_5;
            }
            
            
        Image {
                source: "qrc:/fxplugin/images/template_image.png";
                height: 379;
                x: -154;
                width: 330;
                y: -31;
                id: image_1;
            }Image {
                width: 293;
                x: 33;
                height: 172;
                y: 39;
                source: "qrc:/fxplugin/images/template_image.png";
                id: image_2;
            }}
        
    }
    Rectangle {
        width: 238;
        height: 302;
        x: 360;
        y: 42;
        color: "#009920";
        id: rectangle_2;
    Image {
            source: "qrc:/fxplugin/images/template_image.png";
            x: -40;
            y: 12;
            width: 281;
            height: 280;
            id: image_3;
        }}
    Rectangle {
        height: 206;
        width: 276;
        color: "#009920";
        x: 4;
        y: 234;
        id: rectangle_4;
    }
    Text {
        text: "text";
        width: 80;
        height: 20;
        x: 71;
        y: 15;
        id: text_1;
    }
}
