/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "welcomeplugin.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/iwelcomepage.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/modemanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <utils/theme/theme.h>

#include <QVBoxLayout>
#include <QMessageBox>

#include <QDir>
#include <QOpenGLContext>
#include <QQmlPropertyMap>

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

using namespace Core;
using namespace ExtensionSystem;
using namespace Utils;

static const char currentPageSettingsKeyC[] = "WelcomeTab";

namespace Welcome {
namespace Internal {

static QString applicationDirPath()
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return FileUtils::normalizePathName(QCoreApplication::applicationDirPath());
}

static QString resourcePath()
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return FileUtils::normalizePathName(ICore::resourcePath());
}

class WelcomeMode : public IMode
{
    Q_OBJECT
    Q_PROPERTY(int activePlugin READ activePlugin WRITE setActivePlugin NOTIFY activePluginChanged)
public:
    WelcomeMode();
    ~WelcomeMode();

    void activated();
    void initPlugins();
    int activePlugin() const { return m_activePlugin; }

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

private:
    void welcomePluginAdded(QObject*);
    void sceneGraphError(QQuickWindow::SceneGraphError, const QString &message);
    void facilitateQml(QQmlEngine *engine);
    void addPages(const QList<IWelcomePage *> &pages);

    QWidget *m_modeWidget;
    QuickContainer *m_welcomePage;
    QMap<Id, IWelcomePage *> m_idPageMap;
    QList<IWelcomePage *> m_pluginList;
    int m_activePlugin;
    QQmlPropertyMap m_themeProperties;
};

WelcomeMode::WelcomeMode()
    : m_activePlugin(0)
{
    setDisplayName(tr("Welcome"));
    setIcon(QIcon(QLatin1String(":/welcome/images/mode_welcome.png")));
    setPriority(Constants::P_MODE_WELCOME);
    setId(Constants::MODE_WELCOME);
    setContextHelpId(QLatin1String("Qt Creator Manual"));
    setContext(Context(Constants::C_WELCOME_MODE));

    m_modeWidget = new QWidget;
    m_modeWidget->setObjectName(QLatin1String("WelcomePageModeWidget"));
    QVBoxLayout *layout = new QVBoxLayout(m_modeWidget);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_welcomePage = new QuickContainer();
    onThemeChanged(); // initialize background color and theme properties
    m_welcomePage->setResizeMode(QuickContainer::SizeRootObjectToView);

    m_welcomePage->setObjectName(QLatin1String("WelcomePage"));

    connect(m_welcomePage, &QuickContainer::sceneGraphError,
            this, &WelcomeMode::sceneGraphError);

    StyledBar *styledBar = new StyledBar(m_modeWidget);
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

    connect(ICore::instance(), &ICore::themeChanged, this, &WelcomeMode::onThemeChanged);

    setWidget(m_modeWidget);
}

void WelcomeMode::onThemeChanged()
{
    const QVariantHash creatorTheme = Utils::creatorTheme()->values();
    for (auto it = creatorTheme.constBegin(); it != creatorTheme.constEnd(); ++it)
        m_themeProperties.insert(it.key(), it.value());
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = ICore::settings();
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

void WelcomeMode::facilitateQml(QQmlEngine *engine)
{
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

    QQmlContext *ctx = engine->rootContext();
    ctx->setContextProperty(QLatin1String("welcomeMode"), this);

    ctx->setContextProperty(QLatin1String("creatorTheme"), &m_themeProperties);

#if defined(USE_QUICK_WIDGET) && (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
    bool useNativeText = !HostOsInfo::isMacHost();
#else
    bool useNativeText = true;
#endif
    ctx->setContextProperty(QLatin1String("useNativeText"), useNativeText);
}

void WelcomeMode::initPlugins()
{
    QSettings *settings = ICore::settings();
    setActivePlugin(settings->value(QLatin1String(currentPageSettingsKeyC)).toInt());

    facilitateQml(m_welcomePage->engine());

    QList<IWelcomePage *> availablePages = PluginManager::getObjects<IWelcomePage>();
    addPages(availablePages);
    // make sure later added pages are made available too:
    connect(PluginManager::instance(), &PluginManager::objectAdded,
            this, &WelcomeMode::welcomePluginAdded);

    QString path = resourcePath() + QLatin1String("/welcomescreen/welcomescreen.qml");

    // finally, load the root page
    m_welcomePage->setSource(QUrl::fromLocalFile(path));
}

void WelcomeMode::welcomePluginAdded(QObject *obj)
{
    IWelcomePage *page = qobject_cast<IWelcomePage*>(obj);
    if (!page)
        return;
    addPages(QList<IWelcomePage *>() << page);
}

void WelcomeMode::addPages(const QList<IWelcomePage *> &pages)
{
    QList<IWelcomePage *> addedPages = pages;
    Utils::sort(addedPages, [](const IWelcomePage *l, const IWelcomePage *r) {
        return l->priority() < r->priority();
    });
    // insert into m_pluginList, keeping m_pluginList sorted by priority
    QQmlEngine *engine = m_welcomePage->engine();
    auto addIt = addedPages.begin();
    auto currentIt = m_pluginList.begin();
    while (addIt != addedPages.end()) {
        IWelcomePage *page = *addIt;
        QTC_ASSERT(!m_idPageMap.contains(page->id()), ++addIt; continue);
        while (currentIt != m_pluginList.end() && (*currentIt)->priority() <= page->priority())
            ++currentIt;
        // currentIt is now either end() or a page with higher value
        currentIt = m_pluginList.insert(currentIt, page);
        m_idPageMap.insert(page->id(), page);
        page->facilitateQml(engine);
        ++currentIt;
        ++addIt;
    }
    // update model through reset
    QQmlContext *ctx = engine->rootContext();
    ctx->setContextProperty(QLatin1String("pagesModel"), QVariant::fromValue(
                                Utils::transform(m_pluginList, // transform into QList<QObject *>
                                                 [](IWelcomePage *page) -> QObject * {
                                    return page;
                                })));
}

WelcomePlugin::WelcomePlugin()
  : m_welcomeMode(0)
{
}

bool WelcomePlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    if (!QOpenGLContext().create()) {
        *errorMessage = tr("Cannot create OpenGL context.");
        return false;
    }

    m_welcomeMode = new WelcomeMode;
    addAutoReleasedObject(m_welcomeMode);

    return true;
}

void WelcomePlugin::extensionsInitialized()
{
    m_welcomeMode->initPlugins();
    ModeManager::activateMode(m_welcomeMode->id());
}

} // namespace Internal
} // namespace Welcome

#include "welcomeplugin.moc"
