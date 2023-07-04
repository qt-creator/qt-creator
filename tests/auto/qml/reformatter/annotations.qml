import QtQuick

@Annotation {}
Item {
    // properties
    property int foo
    property alias bar: x
                        @Annotation2 {
                            someproperty: 10
                        }
    id: someId

        @Annotation3 {
            someproperty: 10
        }
    Rectangle {
        // properties
        property int foo
        property alias bar: x
        id: someId2
    }
}
