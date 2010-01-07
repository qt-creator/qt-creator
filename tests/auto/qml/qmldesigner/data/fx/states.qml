import Qt 4.6

Item {
    id: theRootItem;
    width: 400;
    height: 400;
    Rectangle {
        width: 157;
        height: 120;
        x: 145;
        y: 157;
        id: Rectangle_1;
    }

    Rectangle {
        x: 49;
        y: 6;
        width: 100;
        id: Rectangle_2;
        color: "#009920";
        height: 100;
    }

    states: [
        State {
            name: "State1";
            when: destination === "one";

            PropertyChanges {
                target: Rectangle_2;
                height: 200
                width: 300
            }
        },
        State {
            name: "State2";
            when: false == true;
        },
        State {
            name: "State3";
            PropertyChanges {
                target: Rectangle_2;
                x: 200
                y: 300
            }
        }
    ]

    transitions: [
        Transition {
            NumberAnimation {
                properties: "width, height"
                easing: "OutQuad"
                duration: 500
            }
        }
    ]
}

