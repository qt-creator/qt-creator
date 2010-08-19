import Qt 4.7

// Try to look up prop in child.
// It should get the root's prop.

Rectangle {
    id: theRoot
    property variant prop
    Item {
        id: theParent
        property variant prop
        Item {
            id: theChild
        }
    }
}
