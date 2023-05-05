import QtQml

QtObject {
    Component.onCompleted: {
        for (var i of ["one", "two", "free"]) {
            console.debug(i)
        }
    }
}
