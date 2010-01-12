import Qt 4.6

// Try to look up root, parentItem, foo, child and childChild
// in all symbol contexts. It should always get the right item.

Item {
    id: root
    Script { 
      function root() {}
      function parentItem() {}
      function foo() {}
      function child() {}
      function childChild() {}
    }
    property var root: "wrong";
    property var parentItem: "wrong";
    property var foo: "wrong";
    property var child: "wrong";
    property var childChild: "wrong";
    Item {
        id: parentItem
        property var root: "wrong";
        property var parentItem: "wrong";
        property var foo: "wrong";
        property var child: "wrong";
        property var childChild: "wrong";
        Item {
            id: foo
            property var root: "wrong";
            property var parentItem: "wrong";
            property var foo: "wrong";
            property var child: "wrong";
            property var childChild: "wrong";
            Item {
                id: child
                property var root: "wrong";
                property var parentItem: "wrong";
                property var foo: "wrong";
                property var child: "wrong";
                property var childChild: "wrong";
                Item {
                    id: childChild
                }
            }
        }
    }
}