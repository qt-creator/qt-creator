#include "qmlprofilerplugin.h"
#include "qmlprofilerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <analyzerbase/analyzermanager.h>

#include <QtGui/QAction>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>

#include <QtCore/QtPlugin>

#include "qmlprofilertool.h"

using namespace Analyzer;
using namespace QmlProfiler::Internal;

QmlProfilerPlugin *QmlProfilerPlugin::m_instance = 0;
bool QmlProfilerPlugin::debugOutput = false;


class QmlProfilerPlugin::QmlProfilerPluginPrivate
{
public:
    QmlProfilerPluginPrivate(QmlProfilerPlugin *qq):
        q(qq)
    {}

    void initialize(const QStringList &arguments, QString *errorString);

    QmlProfilerPlugin *q;
};

void QmlProfilerPlugin::QmlProfilerPluginPrivate::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

//    AnalyzerManager::instance()->addTool(new MemcheckTool(this));
}



QmlProfilerPlugin::QmlProfilerPlugin()
    : d(new QmlProfilerPluginPrivate(this))
{
    m_instance = this;
}

QmlProfilerPlugin::~QmlProfilerPlugin()
{
    delete d;
    m_instance = 0;
}

bool QmlProfilerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    d->initialize(arguments, errorString);

    AnalyzerManager::instance()->addTool(new QmlProfilerTool(this));

    return true;
}

void QmlProfilerPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // "In the extensionsInitialized method, a plugin can be sure that all
    //  plugins that depend on it are completely initialized."
}

ExtensionSystem::IPlugin::ShutdownFlag QmlProfilerPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

QmlProfilerPlugin *QmlProfilerPlugin::instance()
{
    return m_instance;
}

Q_EXPORT_PLUGIN(QmlProfilerPlugin)

