import QmlProject 1.3

Project {
    // importPaths: ["."] // optional extra import paths
    // projectRootPath: "." // optional root path relative to qmlproject file path

    /* Global configuration */
    MCU.Config {
        controlsStyle: "QtQuick.Controls.StyleDefault"
        debugBytecode: false
        debugLineDirectives: false
        binaryAssetOptions: "Automatic"
        // platformImageAlignment: 1 // undefined by default
        platformPixelWidthAlignment: 1
        platformAlphaPixelFormat: "ARGB8888"
        platformOpaquePixelFormat: "XRGB8888"
        platformAlphaCompressedLosslessResourcePixelFormat: "ARGB8888RLE"
        platformOpaqueCompressedLosslessResourcePixelFormat: "RGB888RLE"
        // maxResourceCacheSize: 102400 // undefined by default

        // global defaults for image properties
        resourceAlphaOptions: "ForTransformations"
        resourceRlePremultipliedAlpha: true
        resourceImagePixelFormat: "Automatic"
        resourceCachePolicy: "OnStartup"
        resourceCompression: false
        resourceStorageSection: "QulResourceData"
        resourceRuntimeAllocationType: 3
        resourceOptimizeForRotation: false
        resourceOptimizeForScale: false

        // default font engine selection
        fontEngine: "Static" // alternative option: "Spark"

        // font defaults for both engines
        defaultFontFamily: "DejaVu Sans"
        defaultFontQuality: "VeryHigh"
        glyphsCachePolicy: "OnStartup"
        glyphsStorageSection: "QulFontResourceData"
        glyphsRuntimeAllocationType: 3

        // font defaults for "Static"
        autoGenerateGlyphs: true
        complexTextRendering: true
        fontFilesCachePolicy: "OnStartup"
        fontFilesStorageSection: "QulFontResourceData"
        fontFilesRuntimeAllocationType: 3

        // font properties for "Spark"
        fontCachePriming: true
        fontCacheSize: 104800
        fontHeapSize: -1
        fontHeapPrealloc: true
        fontCachePrealloc: true
        fontVectorOutlinesDrawing: false
        maxParagraphSize: 100
    }

    /* QML files */
    // optional root property for adding one single qml file
    // mainFile: "%{MainQmlFile}"
    QmlFiles {
        files: ["%{MainQmlFile}"]
        MCU.copyQmlFiles: false
    }

    /* Images */
    ImageFiles {
        // files: [""] // uncomment and add image files
        MCU.base: "images" // example base "images".
        MCU.prefix: "pics" // example prefix "pics".

        MCU.resourceAlphaOptions: "ForTransformations"
        MCU.resourceRlePremultipliedAlpha: true
        MCU.resourceImagePixelFormat: "Automatic"
        MCU.resourceCachePolicy: "OnStartup"
        MCU.resourceCompression: false
        MCU.resourceStorageSection: "QulResourceData"
        MCU.resourceRuntimeAllocationType: 3
        MCU.resourceOptimizeForRotation: false
        MCU.resourceOptimizeForScale: false
    }

    /* Modules */
    ModuleFiles {
        files: ["%{ModuleFile}"]
        // qulModules: [ // Uncomment for adding Qul modules
            // "Qul::Controls",
            // "Qul::ControlsTemplates",
            // "Qul::Shapes",
            // "Qul::Timeline"
        // ]
    }

    /* Interfaces */
    InterfaceFiles {
        // files: [""] // uncomment for adding header files
        MCU.qmlImports: ["QtQuick"]
    }

    /* Translations */
    TranslationFiles {
        // files: [""] // Uncomment for adding translation files with .ts extension
        MCU.omitSourceLanguage: false
    }

    FontFiles {
        // files: [""] // Uncomment for adding font files (.ttf, .otf, .pfa, .ttc, .pfb)
        MCU.addDefaultFonts: true
    }

    FontConfiguration {
        // font engine selection overriddes default
        MCU.fontEngine: "Static" // or "Spark"

        // properties shared between both engines
        MCU.defaultFontFamily: "DejaVu Sans"
        MCU.defaultFontQuality: "VeryHigh"
        MCU.glyphsCachePolicy: "OnStartup"
        MCU.glyphsStorageSection: "QulFontResourceData"
        MCU.glyphsRuntimeAllocationType: 3

        // properties for Static engine
        MCU.autoGenerateGlyphs: true
        MCU.complexTextRendering: true
        MCU.fontFilesCachePolicy: "OnStartup"
        MCU.fontFilesStorageSection: "QulFontResourceData"
        MCU.fontFilesRuntimeAllocationType: 3

        // monotype for Spark engine
        MCU.fontCachePriming: true
        MCU.fontCacheSize: 104800
        MCU.fontHeapSize: -1
        MCU.fontHeapPrealloc: true
        MCU.fontCachePrealloc: true
        MCU.fontVectorOutlinesDrawing: false
        MCU.maxParagraphSize: 100
    }
}
