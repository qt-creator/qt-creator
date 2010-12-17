Item {
    transitions: [
    //! [first transition]
        Transition {
            from: "*"; to: "State1"
            NumberAnimation {
                properties: "x,y";
                duration: 1000
            }
        },
    //! [first transition]
    //! [second transition]
        Transition {
            from: "*"; to: "State2"
            NumberAnimation {
                properties: "x,y";
                easing.type: Easing.InOutQuad;
                duration: 2000
            }
        },
    //! [second transition]
    //! [default transition]
        Transition {
            NumberAnimation {
                properties: "x,y";
                duration: 200
            }
        }
    ]
    //! [default transition]
}
