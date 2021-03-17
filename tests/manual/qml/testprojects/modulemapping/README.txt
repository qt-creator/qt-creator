This is a test for the module mapping feature used by Qt for MCUs.

Please add this source directory to the QML_IMPORT_PATH! A Qt for MCUs kit will do this automatically, but other kits
won't.

You can check that it works by going to test.qml, and "myproperty" should not be underligned as error. Without mapping,
the use of Button would resolve to QtQuick.Control's Button, which doesn't have that property. With the mapping, it
redirects to MyControls's Button which does have the property. You can verify this by control/command-clicking on the
property. This should take you to MyControls/Button.qml.
