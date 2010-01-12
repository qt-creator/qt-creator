import Qt 4.6

// Try to look up rootFunc, parentFunc, childFunc
// in all symbol contexts. It should always get the function.

Item {
    id: theRoot
    Script { 
        function rootFunc() {}
    }
    property var rootFunc: "wrong";
    property var parentFunc: "wrong";
    property var childFunc: "wrong";
    Item {
        id: theParent
        Script { 
            function parentFunc() {}
        }
        property var rootFunc: "wrong";
        property var parentFunc: "wrong";
        property var childFunc: "wrong";
        Item {
            id: theChild
            Script { 
                function childFunc() {}
            }
            property var rootFunc: "wrong";
            property var parentFunc: "wrong";
            property var childFunc: "wrong";
        }
    }
}