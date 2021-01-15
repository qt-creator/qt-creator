import QtQuick 2.15

Rectangle {
    Component.onCompleted: {
        const values = ["10", "20", "30", "40"]
        const result = values.map(value => parseInt(value))
        const result2 = values.map(value => parseInt(value))
        const result3 = values.map(value => {
                                       return parseInt(value)
                                   })
        const result4 = values.map((value, i) => parseInt(value) + i)
        const result5 = values.map((value, i) => {
                                       return parseInt(value) + i
                                   })
    }
}
