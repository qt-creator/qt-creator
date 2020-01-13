import qbs

QtcPlugin {
    name: "Marketplace"

    Depends { name: "Core" }
    Depends { name: "Utils" }

    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.network" }

    files: [
        "marketplaceplugin.h",
        "productlistmodel.cpp", "productlistmodel.h",
        "qtmarketplacewelcomepage.cpp", "qtmarketplacewelcomepage.h",
    ]
}
