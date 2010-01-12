import Qt 4.6

// Try to look up prop in child.
// It should get the root's prop.

Item {
    id: theRoot
    property var prop
    Item {
        id: theParent
        property var prop
        Item {
            id: theChild
        }
    }
}
