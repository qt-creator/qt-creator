/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "welcomeplugin.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/dialogs/iwizard.h>

#include <projectexplorer/projectexplorer.h>

#include <utils/hostosinfo.h>
#include <utils/styledbar.h>
#include <utils/iwelcomepage.h>
#include <utils/networkaccessmanager.h>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

#include <QScrollArea>
#include <QDesktopServices>
#include <QPainter>
#include <QVBoxLayout>

#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QUrl>
#include <QtPlugin>

#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDeclarativeNetworkAccessManagerFactory>

enum { debug = 0 };

using namespace ExtensionSystem;
using namespace Utils;

static const char currentPageSettingsKeyC[] = "WelcomeTab";

namespace Welcome {
namespace Internal {

class NetworkAccessManagerFactory : public QDeclarativeNetworkAccessManagerFactory
{
public:
    NetworkAccessManagerFactory(): QDeclarativeNetworkAccessManagerFactory() {}
    QNetworkAccessManager* create(QObject *parent) { return new Utils::NetworkAccessManager(parent); }
};


struct WelcomeModePrivate
{

};

class WelcomeMode : public Core::IMode
{
    Q_OBJECT
    Q_PROPERTY(int activePlugin READ activePlugin WRITE setActivePlugin NOTIFY activePluginChanged)
public:
    WelcomeMode();
    ~WelcomeMode();

    void activated();
    void initPlugins();
    int activePlugin() const { return m_activePlugin; }

    Q_SCRIPTABLE QString platform() const;

    bool eventFilter(QObject *, QEvent *);
public slots:
    void newProject();
    void openProject();

    void setActivePlugin(int pos)
    {
        if (m_activePlugin != pos) {
            m_activePlugin = pos;
            emit activePluginChanged(pos);
        }
    }

signals:
    void activePluginChanged(int pos);

private slots:
    void welcomePluginAdded(QObject*);

private:
    void facilitateQml(QDeclarativeEngine *engine);

    QWidget *m_modeWidget;
    QDeclarativeView *m_welcomePage;
    QList<QObject*> m_pluginList;
    int m_activePlugin;
    NetworkAccessManagerFactory *m_networkAccessManagerFactory;
};

// ---  WelcomeMode
WelcomeMode::WelcomeMode() :
    m_activePlugin(0)
    , m_networkAccessManagerFactory(new NetworkAccessManagerFactory)
{
    setDisplayName(tr("Welcome"));
    setIcon(QIcon(QLatin1String(Core::Constants::ICON_QTLOGO_32)));
    setPriority(Core::Constants::P_MODE_WELCOME);
    setId(Core::Constants::MODE_WELCOME);
    setType(Core::Constants::MODE_WELCOME_TYPE);
    setContextHelpId(QLatin1String("Qt Creator Manual"));
    setContext(Core::Context(Core::Constants::C_WELCOME_MODE));

    m_welcomePage = new QDeclarativeView;
    m_welcomePage->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    // filter to forward dragEnter events
    m_welcomePage->installEventFilter(this);
    m_welcomePage->viewport()->installEventFilter(this);

    m_modeWidget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_modeWidget->setLayout(layout);

    Utils::StyledBar* styledBar = new Utils::StyledBar(m_modeWidget);
    layout->addWidget(styledBar);
    QScrollArea *scrollArea = new QScrollArea(m_modeWidget);
    scrollArea->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scrollArea);
    scrollArea->setWidget(m_welcomePage);
    scrollArea->setWidgetResizable(true);
    m_welcomePage->setMinimumWidth(880);
    m_welcomePage->setMinimumHeight(548);
    PluginManager *pluginManager = PluginManager::instance();
    connect(pluginManager, SIGNAL(objectAdded(QObject*)), SLOT(welcomePluginAdded(QObject*)));

    setWidget(m_modeWidget);
}

bool WelcomeMode::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::DragEnter) {
        e->ignore();
        return true;
    }
    return false;
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = Core::ICore::settings();
    settings->setValue(QLatin1String(currentPageSettingsKeyC), activePlugin());
    delete m_modeWidget;
    delete m_networkAccessManagerFactory;
}

bool sortFunction(Utils::IWelcomePage * a, Utils::IWelcomePage *b)
{
    return a->priority() < b->priority();
}

void WelcomeMode::facilitateQml(QDeclarativeEngine * /*engine*/)
{
}

static QString applicationDirPath()
{
#ifdef Q_OS_WIN
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return Utils::normalizePathName(QCoreApplication::applicationDirPath());
#else
    return QCoreApplication::applicationDirPath();
#endif
}

static QString resourcePath()
{
#ifdef Q_OS_WIN
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return Utils::normalizePathName(Core::ICore::resourcePath());
#else
    return Core::ICore::resourcePath();
#endif
}

void WelcomeMode::initPlugins()
{
    QSettings *settings = Core::ICore::settings();
    setActivePlugin(settings->value(QLatin1String(currentPageSettingsKeyC)).toInt());

    // TODO: re-enable reading from Settings when possible. See QTCREATORBUG-6803
    if (activePlugin() > 1)
        setActivePlugin(1);

    QDeclarativeContext *ctx = m_welcomePage->rootContext();
    ctx->setContextProperty(QLatin1String("welcomeMode"), this);

    QList<Utils::IWelcomePage*> duplicatePlugins = PluginManager::getObjects<Utils::IWelcomePage>();
    qSort(duplicatePlugins.begin(), duplicatePlugins.end(), &sortFunction);

    QList<Utils::IWelcomePage*> plugins;
    QHash<Utils::IWelcomePage::Id, Utils::IWelcomePage*> pluginHash;

    //avoid duplicate ids - choose by priority
    foreach (Utils::IWelcomePage* plugin, duplicatePlugins) {
        if (pluginHash.contains(plugin->id())) {
            Utils::IWelcomePage* pluginOther = pluginHash.value(plugin->id());

            if (pluginOther->priority() > plugin->priority()) {
                plugins.removeAll(pluginOther);
                pluginHash.remove(pluginOther->id());
                plugins << plugin;
                pluginHash.insert(plugin->id(), plugin);
            }

        } else {
            plugins << plugin;
            pluginHash.insert(plugin->id(), plugin);
        }
    }


    QDeclarativeEngine *engine = m_welcomePage->engine();
    QStringList importPathList = engine->importPathList();
    importPathList << resourcePath() + QLatin1String("/welcomescreen");
    engine->setImportPathList(importPathList);
    if (!debug)
        engine->setOutputWarningsToStandardError(false);
    engine->setNetworkAccessManagerFactory(m_networkAccessManagerFactory);
    QString pluginPath = applicationDirPath();
    if (HostOsInfo::isMacHost())
        pluginPath += QLatin1String("/../PlugIns");
    else
        pluginPath += QLatin1String("/../" IDE_LIBRARY_BASENAME "/qtcreator");
    engine->addImportPath(QDir::cleanPath(pluginPath));
    facilitateQml(engine);
    foreach (Utils::IWelcomePage *plugin, plugins) {
        plugin->facilitateQml(engine);
        m_pluginList.append(plugin);
    }

    ctx->setContextProperty(QLatin1String("pagesModel"), QVariant::fromValue(m_pluginList));

    QString path = resourcePath() + QLatin1String("/welcomescreen/welcomescreen.qml");

    // finally, load the root page
    m_welcomePage->setSource(
            QUrl::fromLocalFile(path));
}

QString WelcomeMode::platform() const
{
    switch (HostOsInfo::hostOs()) {
    case HostOsInfo::HostOsWindows: return QLatin1String("windows");
    case HostOsInfo::HostOsMac: return QLatin1String("mac");
    case HostOsInfo::HostOsLinux: return QLatin1String("linux");
    case HostOsInfo::HostOsOtherUnix: return QLatin1String("unix");
    default: return QLatin1String("other");
    }
}

void WelcomeMode::welcomePluginAdded(QObject *obj)
{
    QHash<Utils::IWelcomePage::Id, Utils::IWelcomePage*> pluginHash;

    foreach (QObject *obj, m_pluginList) {
        Utils::IWelcomePage *plugin = qobject_cast<Utils::IWelcomePage*>(obj);
        pluginHash.insert(plugin->id(), plugin);
    }
    if (Utils::IWelcomePage *plugin = qobject_cast<Utils::IWelcomePage*>(obj)) {
        //check for duplicated id
        if (pluginHash.contains(plugin->id())) {
            Utils::IWelcomePage* pluginOther = pluginHash.value(plugin->id());

            if (pluginOther->priority() > plugin->priority())
                m_pluginList.removeAll(pluginOther);
            else
                return;
        }

        int insertPos = 0;
        foreach (Utils::IWelcomePage* p, PluginManager::getObjects<Utils::IWelcomePage>()) {
            if (plugin->priority() < p->priority())
                insertPos++;
            else
                break;
        }
        m_pluginList.insert(insertPos, plugin);
        // update model through reset
        QDeclarativeContext *ctx = m_welcomePage->rootContext();
        ctx->setContextProperty(QLatin1String("pagesModel"), QVariant::fromValue(m_pluginList));
    }
}

void WelcomeMode::newProject()
{
    Core::ICore::showNewItemDialog(tr("New Project"),
                                               Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
}

void WelcomeMode::openProject()
{
    ProjectExplorer::ProjectExplorerPlugin::instance()->openOpenProjectDialog();
}

WelcomePlugin::WelcomePlugin()
  : m_welcomeMode(0)
{
}

/*! Initializes the plugin. Returns true on success.
    Plugins want to register objects with the plugin manager here.

    \a errorMessage can be used to pass an error message to the plugin system,
       if there was any.
*/
bool WelcomePlugin::initialize(const QStringList & /* arguments */, QString * /* errorMessage */)
{
    m_welcomeMode = new WelcomeMode;
    addAutoReleasedObject(m_welcomeMode);

    return true;
}

/*! Notification that all extensions that this plugin depends on have been
    initialized. The dependencies are defined in the plugins .qwp file.

    Normally this method is used for things that rely on other plugins to have
    added objects to the plugin manager, that implement interfaces that we're
    interested in. These objects can now be requested through the
    PluginManagerInterface.

    The WelcomePlugin doesn't need things from other plugins, so it does
    nothing here.
*/
void WelcomePlugin::extensionsInitialized()
{
    m_welcomeMode->initPlugins();
    Core::ModeManager::activateMode(m_welcomeMode->id());
}

} // namespace Internal
} // namespace Welcome


Q_EXPORT_PLUGIN(Welcome::Internal::WelcomePlugin)

#include "welcomeplugin.moc"
