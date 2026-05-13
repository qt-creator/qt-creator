project('%{ProjectName}', 'cpp',
@if %{IsStatic}
        default_options : ['cpp_std=c++17', 'default_library=static'],
@else
        default_options : ['cpp_std=c++17', 'default_library=shared'],
@endif
        meson_version:'>=0.48')

@if '%{QtModule}' != 'none'
# Documentation: https://mesonbuild.com/Qt6-module.html
qt6 = import('qt6')

@if '%{QtModule}' == 'core'
qt6dep = dependency('qt6', modules : ['Core'])
@endif
@if '%{QtModule}' == 'gui'
qt6dep = dependency('qt6', modules : ['Core', 'Gui'])
@endif
@if '%{QtModule}' == 'widgets'
qt6dep = dependency('qt6', modules : ['Core', 'Gui', 'Widgets'])
@endif

@if %{HasTranslation}
translations = qt6.compile_translations(ts_files : '%{TsFileName}', build_by_default : true)
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

moc_files = qt6.preprocess(moc_headers : [
                            @if '%{Type}' === 'shared'
                            '%{GlobalHdrFileName}',
                            @endif
                            '%{HdrFileName}'],
                            moc_extra_arguments : cpp_args,
                            dependencies: [qt6dep])
@endif


library('%{ProjectName}', '%{SrcFileName}'
@if '%{QtModule}' != 'none'
        , moc_files
        , dependencies : [qt6dep]
        , cpp_args : cpp_args
@endif
        , install : true)
