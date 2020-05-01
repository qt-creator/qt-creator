project('mesonsampleproject', 'cpp',default_options : ['cpp_std=c++11'])

# Documentation: https://mesonbuild.com/Qt5-module.html
qt5 = import('qt5')
qt5dep = dependency('qt5', modules : ['Core', 'Widgets'])

translations = qt5.compile_translations(ts_files : 'mesonsampleproject_fr_FR.ts', build_by_default : true)

generated_files = qt5.preprocess(
                            moc_headers : 'mesonsampleproject.h',
                            ui_files : 'mesonsampleproject.ui',
                            dependencies: [qt5dep])

executable('mesonsampleproject'
        , 'main.cpp'
        , 'mesonsampleproject.cpp'
        , generated_files
        , dependencies : [qt5dep]
        , install : true)
