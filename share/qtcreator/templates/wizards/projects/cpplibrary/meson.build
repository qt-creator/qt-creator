project('%{ProjectName}', 'cpp',
@if %{IsStatic}
        default_options : ['cpp_std=c++17', 'default_library=static'],
@else
        default_options : ['cpp_std=c++17', 'default_library=shared'],
@endif
        meson_version:'>=0.48')

@if '%{QtModule}' != 'none'
# Documentation: https://mesonbuild.com/Qt5-module.html
qt5 = import('qt5')

@if '%{QtModule}' == 'core'
qt5dep = dependency('qt5', modules : ['Core'])
@endif
@if '%{QtModule}' == 'gui'
qt5dep = dependency('qt5', modules : ['Core', 'Gui'])
@endif
@if '%{QtModule}' == 'widgets'
qt5dep = dependency('qt5', modules : ['Core', 'Gui', 'Widgets'])
@endif

@if %{HasTranslation}
translations = qt5.compile_translations(ts_files : '%{TsFileName}', build_by_default : true)
@endif

cpp_args = [
@if %{IsShared}
    '-D%{LibraryDefine}'
    @if %{IsQtPlugin}
    ,'-DQT_PLUGIN'
    @endif
@else
    @if %{IsQtPlugin}
    '-DQT_STATICPLUGIN'
    @endif
@endif
]

moc_files = qt5.preprocess(moc_headers : [
                            @if '%{Type}' === 'shared'
                            '%{GlobalHdrFileName}',
                            @endif
                            '%{HdrFileName}'],
                            moc_extra_arguments : cpp_args,
                            dependencies: [qt5dep])
@endif


library('%{ProjectName}', '%{SrcFileName}'
@if '%{QtModule}' != 'none'
        , moc_files
        , dependencies : [qt5dep]
        , cpp_args : cpp_args
@endif
        , install : true)
