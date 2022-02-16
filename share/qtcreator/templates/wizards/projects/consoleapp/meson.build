project('%{ProjectName}', 'cpp',default_options : ['cpp_std=c++17'], meson_version:'>=0.44')

# Documentation: https://mesonbuild.com/Qt5-module.html
qt5 = import('qt5')
qt5core = dependency('qt5', modules : 'Core')

@if %{HasTranslation}
translations = qt5.compile_translations(ts_files : '%{TsFileName}', build_by_default : true)
@endif

executable('%{ProjectName}', '%{CppFileName}', dependencies : [qt5core])
