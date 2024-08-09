import QtQuick
import %{ProjectNameCap}.%{ProjectNameCap}Module.simulation

Item {
    %{Feature}Backend {
        id: backend

        property var settings : IfSimulator.findData(IfSimulator.simulationData, "%{Feature}")

        function initialize() {
            IfSimulator.initializeDefault(settings, backend);
            Base.initialize();
            print("%{Feature} Simulation initialized");
        }
        @if %{Zoned}

        function availableZones() {
            return settings.zones;
        }
        @endif
        @if %{SampleCode}

        @if %{Zoned}
        function processColor(reply, egaColor, zone) {
            print("%{Feature} Simulation processColor() called on zone: " + zone);
            let adjust = zones[zone].gamma
        @else
        function processColor(reply, egaColor) {
            print("%{Feature} Simulation processColor() called");
            let adjust = gamma
        @endif
            if (adjust < 0.0)
                return %{ProjectNameCap}Module.Error;

            let color = %{ProjectNameCap}Module.color();
            let highValue = egaColor & %{ProjectNameCap}Module.Intensity ? 255 : 170;
            let lowValue = egaColor & %{ProjectNameCap}Module.Intensity ? 85 : 0;
            color.red = egaColor & %{ProjectNameCap}Module.Red ? highValue : lowValue;
            color.green = egaColor & %{ProjectNameCap}Module.Green ? highValue : lowValue;
            color.blue = egaColor & %{ProjectNameCap}Module.Blue ? highValue : lowValue;
            if (egaColor == 6)
                color.green = 85;

            color.red = Math.min(255, Math.round(color.red * adjust));
            color.green = Math.min(255, Math.round(color.green * adjust));
            color.blue = Math.min(255, Math.round(color.blue * adjust));
            color.htmlCode = "#" + color.red.toString(16).padStart(2, "0") +
                color.green.toString(16).padStart(2, "0") +
                color.blue.toString(16).padStart(2, "0");

            @if %{Zoned}
            colorProcessed(color, zone);
            zones[zone].processedCount += 1;
            @else
            colorProcessed(color);
            processedCount += 1;
            @endif
            reply.setSuccess(true);
            return %{ProjectNameCap}Module.Ok;
        }
        @endif

        Component.onCompleted: {
            console.log("%{Feature} Simulation created");
        }
    }
}
