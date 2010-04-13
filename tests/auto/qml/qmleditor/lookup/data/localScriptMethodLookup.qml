import Qt 4.7

// Try to look up x, y, z
// in all symbol contexts. It should always get the function.

Item {
    id: theRoot
    Script { 
        function x() {}
    }
    property variant x: "wrong";
    property variant y: "wrong";
    property variant z: "wrong";
    Item {
        id: theParent
        Script { 
            function y() {}
        }
        property variant x: "wrong";
        property variant y: "wrong";
        property variant z: "wrong";
        Item {
            id: theChild
            Script { 
                function z() {}
            }
            property variant x: "wrong";
            property variant y: "wrong";
            property variant z: "wrong";
        }
    }
}