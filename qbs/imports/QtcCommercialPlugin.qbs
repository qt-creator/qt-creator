import qbs

QtcPlugin {
    Depends { name: "LicenseChecker"; required: false }
    cpp.defines: base.concat(LicenseChecker.present ? ["LICENSECHECKER"] : [])
}
