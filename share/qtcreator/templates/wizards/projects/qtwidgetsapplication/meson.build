project('%{ProjectName}', 'cpp',default_options : ['cpp_std=c++17'])

# Documentation: https://mesonbuild.com/Qt6-module.html
qt6 = import('qt6')
qt6dep = dependency('qt6', modules : ['Core', 'Widgets'])

@if %{HasTranslation}
translations = qt6.compile_translations(ts_files : '%{TsFileName}', build_by_default : true)
@endif

generated_files = qt6.preprocess(
                            moc_headers : '%{HdrFileName}',
                            @if %{GenerateForm}
                            ui_files : '%{FormFileName}',
                            @endif
                            dependencies: [qt6dep])

executable('%{ProjectName}'
        , '%{MainFileName}'
        , '%{SrcFileName}'
        , generated_files
        , dependencies : [qt6dep]
        , install : true)
