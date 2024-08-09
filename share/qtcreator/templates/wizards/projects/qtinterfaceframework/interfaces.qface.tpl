module %{ProjectNameCap}.%{ProjectNameCap}Module 1.0;

@if %{Zoned}
@config: { qml_type: "%{Feature}Ui", zoned: true }
@else
@config: { qml_type: "%{Feature}Ui" }
@endif
interface %{Feature} {
    @if %{SampleCode}
    readonly int processedCount;
    real gamma;

    Status processColor(ColorBits egaColor);

    signal colorProcessed(Color colorData);
    @else

    @endif
}
@if %{SampleCode}

flag ColorBits {
    Blue = 1,
    Green = 2,
    Red = 4,
    Intensity = 8
}

struct Color {
    string htmlCode;
    int red;
    int green;
    int blue;
}

enum Status {
    Ok, Error
}
@endif
