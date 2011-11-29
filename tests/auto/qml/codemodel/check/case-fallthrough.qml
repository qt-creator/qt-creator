import QtQuick 2.0

Item {
    x: {
        switch (a) {
        case 1:
        case 2:
            x = 1
            // fallthrough
        case 3:
            break
        case 4:
            continue
        case 5:
        case 6:
        default:
        case 8:
            return
        case 10:
            x = 1
            break
        case 9:
        }
        switch (a) {
        case 1:
        case 2: // 20 9 12
            x = 1
        case 3:
            break
        case 4: // 20 9 12
            function a() {

            }
        case 5:
        case 6:
        default:
        case 8: // 20 9 12
            x = 1
        case 9:
        case 11: // no warning
            x = 1
        }
    }
}
