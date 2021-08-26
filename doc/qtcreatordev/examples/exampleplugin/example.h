#pragma once

#include "example_global.h"

#include <extensionsystem/iplugin.h>

//! [namespaces]
namespace Example {
namespace Internal {
//! [namespaces]

//! [base class]
class ExamplePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Example.json")
//! [base class]

public:
    ExamplePlugin();
    ~ExamplePlugin();

//! [plugin functions]
    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
//! [plugin functions]

//! [slot]
private:
    void triggerAction();
//! [slot]
};

} // namespace Internal
} // namespace Example
