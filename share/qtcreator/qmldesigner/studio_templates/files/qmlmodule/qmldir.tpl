module %{ModuleName}

@if %{HasComponent}

@if %{Singleton}
singleton %{ComponentName} 1.0 %{ComponentName}.qml
@else
%{ComponentName} 1.0 %{ComponentName}.qml
@endif

@endif
