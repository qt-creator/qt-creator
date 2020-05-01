project('%{ProjectName}', 'cpp',default_options : ['cpp_std=c++11'])

# Documentation: https://mesonbuild.com/Qt5-module.html
qt5 = import('qt5')
qt5dep = dependency('qt5', modules : ['Core', 'Widgets'])

@if %{HasTranslation}
translations = qt5.compile_translations(ts_files : '%{TsFileName}', build_by_default : true)
@endif

generated_files = qt5.preprocess(
                            moc_headers : '%{HdrFileName}',
                            @if %{GenerateForm}
                            ui_files : '%{FormFileName}',
                            @endif
                            dependencies: [qt5dep])

executable('%{ProjectName}'
        , '%{MainFileName}'
        , '%{SrcFileName}'
        , generated_files
        , dependencies : [qt5dep]
        , install : true)
