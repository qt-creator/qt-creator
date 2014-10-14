/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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
#include <coreplugin/iwizardfactory.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/styledbar.h>
#include <utils/iwelcomepage.h>

#include <utils/theme/theme.h>
#include <utils/theme/welcometheme.h>

#include <QVBoxLayout>
#include <QMessageBox>

#include <QDir>

#ifdef USE_QUICK_WIDGET
    #include <QtQuickWidgets/QQuickWidget>
    typedef QQuickWidget QuickContainer;
#else
    #include <QtQuick/QQuickView>
    typedef QQuickView QuickContainer;
#endif
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

enum { debug = 0 };

using namespace ExtensionSystem;
using namespace Utils;

static const char currentPageSettingsKeyC[] = "WelcomeTab";

namespace Welcome {
namespace Internal {

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

//    bool eventFilter(QObject *, QEvent *);

public slots:
    void onThemeChanged();

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
    void sceneGraphError(QQuickWindow::SceneGraphError, const QString &message);

private:
    void facilitateQml(QQmlEngine *engine);

    QWidget *m_modeWidget;
    QuickContainer *m_welcomePage;
    QList<QObject*> m_pluginList;
    int m_activePlugin;
    WelcomeTheme *m_welcomeTheme;
};

// ---  WelcomeMode
WelcomeMode::WelcomeMode() :
    m_activePlugin(0),
    m_welcomeTheme(new WelcomeTheme(this))
{
    setDisplayName(tr("Welcome"));
    setIcon(QIcon(QLatin1String(":/welcome/images/mode_welcome.png")));
    setPriority(Core::Constants::P_MODE_WELCOME);
    setId(Core::Constants::MODE_WELCOME);
    setContextHelpId(QLatin1String("Qt Creator Manual"));
    setContext(Core::Context(Core::Constants::C_WELCOME_MODE));

    m_modeWidget = new QWidget;
    m_modeWidget->setObjectName(QLatin1String("WelcomePageModeWidget"));
    QVBoxLayout *layout = new QVBoxLayout(m_modeWidget);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_welcomePage = new QuickContainer();
    onThemeChanged(); //initialize background color
    m_welcomePage->setResizeMode(QuickContainer::SizeRootObjectToView);

    m_welcomePage->setObjectName(QLatin1String("WelcomePage"));

    connect(m_welcomePage, SIGNAL(sceneGraphError(QQuickWindow::SceneGraphError,QString)),
            this, SLOT(sceneGraphError(QQuickWindow::SceneGraphError,QString)));

    Utils::StyledBar* styledBar = new Utils::StyledBar(m_modeWidget);
    styledBar->setObjectName(QLatin1String("WelcomePageStyledBar"));
    layout->addWidget(styledBar);

#ifdef USE_QUICK_WIDGET
    m_welcomePage->setParent(m_modeWidget);
    layout->addWidget(m_welcomePage);
#else
    QWidget *container = QWidget::createWindowContainer(m_welcomePage, m_modeWidget);
    container->setProperty("nativeAncestors", true);
    m_modeWidget->setLayout(layout);
    layout->addWidget(container);
#endif // USE_QUICK_WIDGET

    connect(PluginManager::instance(), SIGNAL(objectAdded(QObject*)), SLOT(welcomePluginAdded(QObject*)));

    setWidget(m_modeWidget);
}

void WelcomeMode::onThemeChanged()
{
    m_welcomePage->setColor(creatorTheme()->color(Theme::BackgroundColorNormal));
    m_welcomeTheme->notifyThemeChanged();
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = Core::ICore::settings();
    settings->setValue(QLatin1String(currentPageSettingsKeyC), activePlugin());
    delete m_modeWidget;
}

void WelcomeMode::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message)
{
    QMessageBox *messageBox =
        new QMessageBox(QMessageBox::Warning,
                        tr("Welcome Mode Load Error"), message,
                        QMessageBox::Close, m_modeWidget);
    messageBox->setModal(false);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->show();
}

void WelcomeMode::facilitateQml(QQmlEngine * /*engine*/)
{
}

static QString applicationDirPath()
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return Utils::FileUtils::normalizePathName(QCoreApplication::applicationDirPath());
}

static QString resourcePath()
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return Utils::FileUtils::normalizePathName(Core::ICore::resourcePath());
}

void WelcomeMode::initPlugins()
{
    QSettings *settings = Core::ICore::settings();
    setActivePlugin(settings->value(QLatin1String(currentPageSettingsKeyC)).toInt());

    QQmlContext *ctx = m_welcomePage->rootContext();
    ctx->setContextProperty(QLatin1String("welcomeMode"), this);

    QList<Utils::IWelcomePage*> duplicatePlugins = PluginManager::getObjects<Utils::IWelcomePage>();
    Utils::sort(duplicatePlugins, [](const Utils::IWelcomePage *l, const Utils::IWelcomePage *r) {
        return l->priority() < r->priority();
    });

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


    QQmlEngine *engine = m_welcomePage->engine();
    QStringList importPathList = engine->importPathList();
    importPathList << resourcePath() + QLatin1String("/welcomescreen");
    engine->setImportPathList(importPathList);
    if (!debug)
        engine->setOutputWarningsToStandardError(false);
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

    connect(creatorTheme(), &Theme::changed, this, &WelcomeMode::onThemeChanged);
    ctx->setContextProperty(QLatin1String("creatorTheme"), m_welcomeTheme);

    // FIXME: pass theme class to QML somehow
    if (creatorTheme()->widgetStyle() == Theme::StyleFlat)
        ctx->setContextProperty(QLatin1String("theme"), QLatin1String("dark"));
    else
        ctx->setContextProperty(QLatin1String("theme"), QLatin1String("default"));

    QString path = resourcePath() + QLatin1String("/welcomescreen/welcomescreen.qml");

    // finally, load the root page
    m_welcomePage->setSource(
            QUrl::fromLocalFile(path));
}

QString WelcomeMode::platform() const
{
    switch (HostOsInfo::hostOs()) {
    case OsTypeWindows: return QLatin1String("windows");
    case OsTypeMac: return QLatin1String("mac");
    case OsTypeLinux: return QLatin1String("linux");
    case OsTypeOtherUnix: return QLatin1String("unix");
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
        QQmlContext *ctx = m_welcomePage->rootContext();
        ctx->setContextProperty(QLatin1String("pagesModel"), QVariant::fromValue(m_pluginList));
    }
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

    Normally this function is used for things that rely on other plugins to have
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

#include "welcomeplugin.moc"
