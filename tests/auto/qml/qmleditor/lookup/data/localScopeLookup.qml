import Qt 4.6

// Try to look up prop in all symbol contexts. 
// It should always get the local one.

Item {
    id: theRoot
    property var prop
    Item {
        id: theParent
        property var prop
        Item {
            id: theChild
            property var prop
        }
    }
}
