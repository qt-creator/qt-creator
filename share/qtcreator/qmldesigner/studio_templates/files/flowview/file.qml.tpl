import QtQuick 2.15
@if %{UseImport}
import %{ApplicationImport}
@endif
import FlowView 1.0

FlowView {
    width: Constants.width
    height: Constants.height

    defaultTransition: FlowTransition {
        id: defaultTransition
    }

@if %{UseEventSimulator}
    EventListSimulator {
        id: eventListSimulator
        active: false
    }
@endif

}
