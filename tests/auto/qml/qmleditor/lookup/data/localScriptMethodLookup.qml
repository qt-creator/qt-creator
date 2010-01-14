import Qt 4.6

// Try to look up x, y, z
// in all symbol contexts. It should always get the function.

Item {
    id: theRoot
    Script { 
        function x() {}
    }
    property var x: "wrong";
    property var y: "wrong";
    property var z: "wrong";
    Item {
        id: theParent
        Script { 
            function y() {}
        }
        property var x: "wrong";
        property var y: "wrong";
        property var z: "wrong";
        Item {
            id: theChild
            Script { 
                function z() {}
            }
            property var x: "wrong";
            property var y: "wrong";
            property var z: "wrong";
        }
    }
}