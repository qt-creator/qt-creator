import Qt 4.6

// Try to look up x, y, z, opacity and visible
// in all symbol contexts. It should always get the right item.

Item {
    id: x
    Script { 
      function x() {}
      function y() {}
      function z() {}
      function opacity() {}
      function visible() {}
    }
    property var x: "wrong";
    property var y: "wrong";
    property var z: "wrong";
    property var opacity: "wrong";
    property var visible: "wrong";
    Item {
        id: y
        property var x: "wrong";
        property var y: "wrong";
        property var z: "wrong";
        property var opacity: "wrong";
        property var visible: "wrong";
        Item {
            id: z
            property var x: "wrong";
            property var y: "wrong";
            property var z: "wrong";
            property var opacity: "wrong";
            property var visible: "wrong";
            Item {
                id: opacity
                property var x: "wrong";
                property var y: "wrong";
                property var z: "wrong";
                property var opacity: "wrong";
                property var visible: "wrong";
                Item {
                    id: visible
                }
            }
        }
    }
}