#ifndef EXAMPLE_H
#define EXAMPLE_H

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
//! [base class]

public:
    ExamplePlugin();
    ~ExamplePlugin();

//! [plugin methods]
    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();
//! [plugin methods]

//! [slot]
private slots:
    void triggerAction();
//! [slot]
};

} // namespace Internal
} // namespace Example

#endif // EXAMPLE_H

