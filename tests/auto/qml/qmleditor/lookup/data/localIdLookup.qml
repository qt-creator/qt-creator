import Qt 4.7

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
    property variant x: "wrong";
    property variant y: "wrong";
    property variant z: "wrong";
    property variant opacity: "wrong";
    property variant visible: "wrong";
    Item {
        id: y
        property variant x: "wrong";
        property variant y: "wrong";
        property variant z: "wrong";
        property variant opacity: "wrong";
        property variant visible: "wrong";
        Item {
            id: z
            property variant x: "wrong";
            property variant y: "wrong";
            property variant z: "wrong";
            property variant opacity: "wrong";
            property variant visible: "wrong";
            Item {
                id: opacity
                property variant x: "wrong";
                property variant y: "wrong";
                property variant z: "wrong";
                property variant opacity: "wrong";
                property variant visible: "wrong";
                Item {
                    id: visible
                }
            }
        }
    }
}