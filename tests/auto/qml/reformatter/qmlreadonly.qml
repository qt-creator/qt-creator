import QtQuick 2.0

Item {
    readonly property int horse: 40
    required property Item theItem
    required data

    function foo() {
        theItem.foo("The issue is exacerbated if the object literal is wrapped onto the next line like so:",
                    {
                        "foo": theFoo
                    })
    }
}
