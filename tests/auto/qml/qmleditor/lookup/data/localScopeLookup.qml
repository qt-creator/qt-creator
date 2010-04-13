import Qt 4.7

// Try to look up prop in all symbol contexts. 
// It should always get the local one.

Item {
    id: theRoot
    property variant prop
    Item {
        id: theParent
        property variant prop
        Item {
            id: theChild
            property variant prop
        }
    }
}
