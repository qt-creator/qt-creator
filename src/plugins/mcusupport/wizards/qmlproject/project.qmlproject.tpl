import QmlProject 1.3

Project {
    qtForMCUs: true // Required by QDS to enable/disable features Supported/Unsupported by QtMCUs projects. Currently ignored by qmlprojectexporter.
    // importPaths: ["imports/CustomModule"] // Alternative API for importing modules.
    // projectRootPath: "." // Optional root path relative to qmlproject file path.
    mainFile: "%{MainQmlFile}" // The application's entrypoint

    /* Global configuration */
    MCU.Config {
        controlsStyle: "QtQuick.Controls.StyleDefault"
        debugBytecode: false
        debugLineDirectives: false

        // maxResourceCacheSize: 0 // Set to 0 by default. Required for OnDemand resource cache policy and resource compression.

        // Global settings for image properties. Can be overridden for selected resources in ImageFiles nodes.
        resourceImagePixelFormat: "Automatic"
        resourceCachePolicy: "NoCaching"
        resourceCompression: false

        // Font engine selection
        fontEngine: "Static" // alternative option: "Spark".

        // Font defaults for both engines
        defaultFontFamily: "DejaVu Sans Mono"
        defaultFontQuality: "VeryHigh"
        glyphsCachePolicy: "NoCaching"
        maxParagraphSize: 100

        // Font properties for "Static"
        addDefaultFonts: false // Set to true to add the default fonts to your project.
        autoGenerateGlyphs: true

        // Font properties for "Spark"
        // These properties are in effect only if the "Spark" font engine is used
        // complexTextRendering: true // Set this to false if complex scripts are not needed (Arabic scripts, Indic scripts, etc.)
        // fontCachePriming: false // Set to true to decrease application startup time. Only applies to fonts configured with unicode ranges (font.unicodeCoverage).
        // fontCacheSize: 0 // If this is needed, use a suitable number. Setting this to a sensible value will improve performance, the global default is 104800.
        // fontHeapSize: -1 // Set to sufficient value to improve performance. -1 means no restrictions to heap allocation.
        // fontHeapPrealloc: true
        // fontCachePrealloc: true
    }

    /* QML files */
    QmlFiles {
        files: ["%{MainQmlFile}"]
        MCU.copyQmlFiles: false
    }

    /* Images */
    ImageFiles {
        files: ["images/icon.png"]
        MCU.base: "images"
        MCU.prefix: "assets"

        MCU.resourceCompression: false
        MCU.resourceImagePixelFormat: "Automatic"
        MCU.resourceOptimizeForRotation: false
        MCU.resourceOptimizeForScale: false

        // Uncomment to override the default values for images in this node
        // MCU.resourceCachePolicy: "NoCaching" // Uncomment to override the default cache policy for these images.
        // MCU.resourceStorageSection: "QulResourceData" // Uncomment to override the default storage section for these images

        // Uncomment the following properties as needed when adding image files for an animated sprite:
        // MCU.resourceAnimatedSprite: true
        // MCU.resourceAnimatedSpriteFrameWidth: 180
        // MCU.resourceAnimatedSpriteFrameHeight: 160
    }

    /* Modules */
    ModuleFiles {
        files: ["imports/CustomModule/%{ModuleFile}"]
        // MCU.qulModules: [ // Uncomment for adding Qul modules
            // "Qul::Controls",
            // "Qul::ControlsTemplates",
            // "Qul::Shapes",
            // "Qul::Timeline"
        // ]
    }

    /* Interfaces */
    InterfaceFiles {
        files: ["src/%{InterfaceFile}"]
        MCU.qmlImports: ["QtQuick"]
    }

    /* Translations */
    TranslationFiles {
        files: ["translations/%{TsFile}"]
        MCU.omitSourceLanguage: false
    }

    FontFiles {
        files: ["fonts/DejaVuSansMono.ttf"]
    }
}
