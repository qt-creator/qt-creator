@if %{Simulator} || %{QtRo} && %{QtRoSimulation}
%{ProjectNameCap}.%{ProjectNameCap}Module:
    config_simulator:
        simulationFile: "qrc:/plugin_resource/simulation.qml"
@if %{Zoned}

%{ProjectNameCap}.%{ProjectNameCap}Module.%{Feature}:
    config_simulator:
        @if %{SampleCode}
        zones: [Foreground, Background]
        @else
        zones: [Zone1, Zone2]
        @endif
@endif
@if %{SampleCode}

%{ProjectNameCap}.%{ProjectNameCap}Module.%{Feature}#processedCount:
    config_simulator:
        @if %{Zoned}
        default: {Foreground: 0, Background: 0}
        @else
        default: 0
        @endif

%{ProjectNameCap}.%{ProjectNameCap}Module.%{Feature}#gamma:
    config_simulator:
        @if %{Zoned}
        default: {Foreground: 1.0, Background: 1.0}
        @else
        default: 1.0
        @endif
@endif
@endif
@if %{QtRo} && %{QtRoProduction}
%{ProjectNameCap}.%{ProjectNameCap}Module:
    config_server_qtro:
        useGeneratedMain: true
@endif
