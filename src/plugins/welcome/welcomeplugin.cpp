/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "welcomeplugin.h"
#include "multifeedrssmodel.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/dialogs/iwizard.h>

#include <projectexplorer/projectexplorer.h>

#include <utils/styledbar.h>
#include <utils/iwelcomepage.h>
#include <utils/networkaccessmanager.h>

#include <QtGui/QScrollArea>
#include <QtGui/QDesktopServices>
#include <QtGui/QPainter>
#include <QtGui/QHBoxLayout>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QtPlugin>

#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeNetworkAccessManagerFactory>

enum { debug = 0 };

using namespace ExtensionSystem;

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
    void sendFeedback();
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
    void modeChanged(Core::IMode*);

private:
    void facilitateQml(QDeclarativeEngine *engine);

    QWidget *m_modeWidget;
    QDeclarativeView *m_welcomePage;
    QHBoxLayout * buttonLayout;
    QList<QObject*> m_pluginList;
    int m_activePlugin;
};

// ---  WelcomeMode
WelcomeMode::WelcomeMode() :
    m_activePlugin(0)
{
    setDisplayName(tr("Welcome"));
    setIcon(QIcon(QLatin1String(Core::Constants::ICON_QTLOGO_32)));
    setPriority(Core::Constants::P_MODE_WELCOME);
    setId(QLatin1String(Core::Constants::MODE_WELCOME));
    setType(QLatin1String(Core::Constants::MODE_WELCOME_TYPE));
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
//    layout->addWidget(new Utils::StyledBar);
    layout->addWidget(m_welcomePage);
    m_modeWidget->setLayout(layout);

    PluginManager *pluginManager = PluginManager::instance();
    connect(pluginManager, SIGNAL(objectAdded(QObject*)), SLOT(welcomePluginAdded(QObject*)));

    Core::ModeManager *modeManager = Core::ICore::instance()->modeManager();
    connect(modeManager, SIGNAL(currentModeChanged(Core::IMode*)), SLOT(modeChanged(Core::IMode*)));

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
    QSettings *settings = Core::ICore::instance()->settings();
    settings->setValue(QLatin1String(currentPageSettingsKeyC), activePlugin());
    delete m_modeWidget;
}

bool sortFunction(Utils::IWelcomePage * a, Utils::IWelcomePage *b)
{
    return a->priority() < b->priority();
}

void WelcomeMode::facilitateQml(QDeclarativeEngine *engine)
{
    static const char feedGroupName[] = "Feeds";

    MultiFeedRssModel *rssModel = new MultiFeedRssModel(this);
    QSettings *settings = Core::ICore::instance()->settings();
    if (settings->childGroups().contains(feedGroupName)) {
        int size = settings->beginReadArray(feedGroupName);
        for (int i = 0; i < size; ++i)
        {
            settings->setArrayIndex(i);
            rssModel->addFeed(settings->value("url").toString());
        }
        settings->endArray();
    } else {
        rssModel->addFeed(QLatin1String("http://labs.trolltech.com/blogs/feed"));
        rssModel->addFeed(QLatin1String("http://feeds.feedburner.com/TheQtBlog?format=xml"));
    }

    engine->rootContext()->setContextProperty("aggregatedFeedsModel", rssModel);
}

void WelcomeMode::initPlugins()
{
    QSettings *settings = Core::ICore::instance()->settings();
    setActivePlugin(settings->value(QLatin1String(currentPageSettingsKeyC)).toInt());

    QDeclarativeContext *ctx = m_welcomePage->rootContext();
    ctx->setContextProperty("welcomeMode", this);

    QList<Utils::IWelcomePage*> plugins = PluginManager::instance()->getObjects<Utils::IWelcomePage>();
    qSort(plugins.begin(), plugins.end(), &sortFunction);

    QDeclarativeEngine *engine = m_welcomePage->engine();
    if (!debug)
        engine->setOutputWarningsToStandardError(false);
    engine->setNetworkAccessManagerFactory(new NetworkAccessManagerFactory);
    QString pluginPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    pluginPath += QLatin1String("/../PlugIns");
#else
    pluginPath += QLatin1String("/../" IDE_LIBRARY_BASENAME "/qtcreator");
#endif
    engine->addImportPath(QDir::cleanPath(pluginPath));
    facilitateQml(engine);
    foreach (Utils::IWelcomePage *plugin, plugins) {
        plugin->facilitateQml(engine);
        m_pluginList.append(plugin);
    }

    ctx->setContextProperty("pagesModel", QVariant::fromValue(m_pluginList));

    // finally, load the root page
    m_welcomePage->setSource(
            QUrl::fromLocalFile(Core::ICore::instance()->resourcePath() + "/welcomescreen/welcomescreen.qml"));
}

QString WelcomeMode::platform() const
{
#if defined(Q_OS_WIN)
    return QLatin1String("windows");
#elif defined(Q_OS_MAC)
    return QLatin1String("mac");
#elif defined(Q_OS_LINUX)
    return QLatin1String("linux");
#elif defined(Q_OS_UNIX)
    return QLatin1String("unix");
#else
    return QLatin1String("other")
#endif
}

void WelcomeMode::welcomePluginAdded(QObject *obj)
{
    if (Utils::IWelcomePage *plugin = qobject_cast<Utils::IWelcomePage*>(obj)) {
        int insertPos = 0;
        foreach (Utils::IWelcomePage* p, PluginManager::instance()->getObjects<Utils::IWelcomePage>()) {
            if (plugin->priority() < p->priority())
                insertPos++;
            else
                break;
        }
        m_pluginList.insert(insertPos, plugin);
        // update model through reset
        QDeclarativeContext *ctx = m_welcomePage->rootContext();
        ctx->setContextProperty("pagesModel", QVariant::fromValue(m_pluginList));
    }
}

void WelcomeMode::sendFeedback()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(
        "http://qt.nokia.com/forms/feedback-forms/qt-creator-user-feedback/view")));
}

void WelcomeMode::newProject()
{
    Core::ICore::instance()->showNewItemDialog(tr("New Project"),
                                               Core::IWizard::wizardsOfKind(Core::IWizard::ProjectWizard));
}

void WelcomeMode::openProject()
{
    ProjectExplorer::ProjectExplorerPlugin::instance()->openOpenProjectDialog();
}

void WelcomeMode::modeChanged(Core::IMode *mode)
{
    Q_UNUSED(mode)

// Eike doesn't like this, but I do...

//    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
//    Core::ModeManager *modeManager = Core::ICore::instance()->modeManager();
//    Core::EditorManager *editorManager = Core::ICore::instance()->editorManager();
//    if (mode->id() == id() && (!projectExplorer->currentProject() && editorManager->openedEditors().isEmpty()))
//        modeManager->setModeBarHidden(true);
//    else
//        modeManager->setModeBarHidden(false);
}

//

WelcomePlugin::WelcomePlugin()
  : m_welcomeMode(0)
{
}

/*! Initializes the plugin. Returns true on success.
    Plugins want to register objects with the plugin manager here.

    \a error_message can be used to pass an error message to the plugin system,
       if there was any.
*/
bool WelcomePlugin::initialize(const QStringList & /* arguments */, QString * /* error_message */)
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
    Core::ModeManager::instance()->activateMode(m_welcomeMode->id());
}

} // namespace Internal
} // namespace Welcome


Q_EXPORT_PLUGIN(Welcome::Internal::WelcomePlugin)

#include "welcomeplugin.moc"
