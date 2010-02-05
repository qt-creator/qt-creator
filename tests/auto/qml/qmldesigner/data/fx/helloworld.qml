import Qt 4.6

Rectangle {
    width: 300
    height: 300
    id: page
    color: "#ffff00"
    Rectangle {
        width: 183
        x: 31
        objectName: "Rectangle_1"
        y: 76
        rotation: -20
        height: 193
        color: "#ffaa00"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#ffaa00" }
            GradientStop { position: 1.0; color: "#ffaa7f" }
        }
        Text {
            width: 78
            x: 11
            y: 10
            height: 20
            id: helloText2
            text: "Hello QmlGraphics!"
        }
        Text {
            width: 85
            x: 11
            objectName: "xt5"
            y: 165
            height: 18
            id: helloText5
            text: "blah!!!!!"
        }
        Image {
            width: 422
            x: 47
            objectName: "Image_1"
            opacity: 0.6
            y: 45.2
            rotation: 0
            scale: 0.2
            height: 498
            source: "http://farm4.static.flickr.com/3002/2722850010_02b62a030f_m.jpg"
            smooth: true
            fillMode: "Tile"
        }
        Text {
            width: 100
            x: 178
            y: 11.64
            rotation: 90
            height: 20
            id: helloText11
            text: "Hello QmlGraphics!"
        }
    }
    Text {
        width: 100
        x: 10
        y: 13
        height: 20
        id: helloText3
        text: "Hello QmlGraphics!"
    }
    Text {
        width: 127
        x: 163
        y: 184
        height: 106
        id: helloText4
        text: "Hello QmlGraphics!"
    }
}
