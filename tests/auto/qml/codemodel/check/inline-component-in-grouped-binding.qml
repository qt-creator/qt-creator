// nDiagnosticMessages=1
import QtQuick

item {
    component Direct: Item {
    }
    component Outer: item {
        component Nested: Item {
        }
    }
}
