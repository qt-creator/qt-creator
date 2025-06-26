import QmlProject

Project
{
    MCU.Module
    {
        uri: "%{ModuleName}"
    }

    QmlFiles
    {
        files: [
@if %{HasComponent}
            "%{ComponentName}.qml"
@endif
        ]
    }
}

