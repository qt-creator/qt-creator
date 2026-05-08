project('%{ProjectName}', 'cpp',default_options : ['cpp_std=c++17'], meson_version:'>=0.44')

# Documentation: https://mesonbuild.com/Qt6-module.html
qt6 = import('qt6')
qt6core = dependency('qt6', modules : 'Core')

@if %{HasTranslation}
translations = qt6.compile_translations(ts_files : '%{TsFileName}', build_by_default : true)
@endif

executable('%{ProjectName}', '%{CppFileName}', dependencies : [qt6core])
