/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "welcomeplugin.h"

#include "communitywelcomepage.h"
#include "ui_welcomemode.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>

#include <utils/styledbar.h>
#include <utils/welcomemodetreewidget.h>
#include <utils/iwelcomepage.h>

#include <QtGui/QScrollArea>
#include <QtGui/QDesktopServices>
#include <QtGui/QToolButton>
#include <QtGui/QPainter>

#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QtPlugin>

enum { debug = 0 };

using namespace ExtensionSystem;

namespace Utils {
    class IWelcomePage;
}

namespace Welcome {
namespace Internal {

// Helper class introduced to cache the scaled background image
// so we avoid re-scaling for every repaint.
class ImageWidget : public QWidget
{
public:
    ImageWidget(const QImage &bg, QWidget *parent) : QWidget(parent), m_bg(bg) {}
    void paintEvent(QPaintEvent *e) {
        if (!m_bg.isNull()) {
            QPainter painter(this);
            if (m_stretch.size() != size())
                m_stretch = QPixmap::fromImage(m_bg.scaled(size(),
                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            if (!m_stretch.size().isEmpty())
                painter.drawPixmap(rect(), m_stretch);
        }
        QWidget::paintEvent(e);
    }
private:
    QImage m_bg;
    QPixmap m_stretch;
};

class WelcomeMode : public Core::IMode
{
    Q_OBJECT

public:
    WelcomeMode();
    ~WelcomeMode();

    void activated();
    void initPlugins();

private slots:
    void slotFeedback();
    void welcomePluginAdded(QObject*);
    void showClickedPage();

private:
    QToolButton *addPageToolButton(Utils::IWelcomePage *plugin, int position = -1);

    typedef QMap<QToolButton *, QWidget *> ToolButtonWidgetMap;

    QScrollArea *m_scrollArea;
    QWidget *m_outerWidget;
    ImageWidget *m_welcomePage;
    ToolButtonWidgetMap buttonMap;
    QHBoxLayout * buttonLayout;
    Ui::WelcomeMode ui;
};

static const char currentPageSettingsKeyC[] = "General/WelcomeTab";

// ---  WelcomeMode
WelcomeMode::WelcomeMode()
{
    setDisplayName(tr("Welcome"));
    setIcon(QIcon(QLatin1String(Core::Constants::ICON_QTLOGO_32)));
    setPriority(Core::Constants::P_MODE_WELCOME);
    setId(QLatin1String(Core::Constants::MODE_WELCOME));
    setType(QLatin1String(Core::Constants::MODE_WELCOME_TYPE));
    setContextHelpId(QLatin1String("Qt Creator Manual"));

    m_outerWidget = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(m_outerWidget);
    l->setMargin(0);
    l->setSpacing(0);
    l->addWidget(new Utils::StyledBar(m_outerWidget));
    m_welcomePage = new ImageWidget(QImage(":/welcome/images/welcomebg.png"), m_outerWidget);
    ui.setupUi(m_welcomePage);
    ui.helpUsLabel->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    ui.feedbackButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    l->addWidget(m_welcomePage);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setFrameStyle(QFrame::NoFrame);
    m_scrollArea->setWidget(m_outerWidget);
    m_scrollArea->setWidgetResizable(true);

    setContext(Core::Context(Core::Constants::C_WELCOME_MODE));
    setWidget(m_scrollArea);

    PluginManager *pluginManager = PluginManager::instance();
    connect(pluginManager, SIGNAL(objectAdded(QObject*)), SLOT(welcomePluginAdded(QObject*)));

    connect(ui.feedbackButton, SIGNAL(clicked()), SLOT(slotFeedback()));
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->setValue(QLatin1String(currentPageSettingsKeyC), ui.stackedWidget->currentIndex());
    delete m_outerWidget;
}

bool sortFunction(Utils::IWelcomePage *a, Utils::IWelcomePage *b)
{
    return a->priority() < b->priority();
}

// Create a QToolButton for a page
QToolButton *WelcomeMode::addPageToolButton(Utils::IWelcomePage *plugin, int position)
{
    QToolButton *btn = new QToolButton;
    btn->setCheckable(true);
    btn->setText(plugin->title());
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btn->setAutoExclusive(true);
    connect (btn, SIGNAL(clicked()), SLOT(showClickedPage()));
    buttonMap.insert(btn, plugin->page());
    if (position >= 0) {
        buttonLayout->insertWidget(position, btn);
    } else {
        buttonLayout->addWidget(btn);
    }
    return btn;
}

void WelcomeMode::initPlugins()
{
    buttonLayout = new QHBoxLayout(ui.navFrame);
    buttonLayout->setMargin(0);
    buttonLayout->setSpacing(0);
    QList<Utils::IWelcomePage*> plugins = PluginManager::instance()->getObjects<Utils::IWelcomePage>();
    qSort(plugins.begin(), plugins.end(), &sortFunction);
    foreach (Utils::IWelcomePage *plugin, plugins) {
        ui.stackedWidget->addWidget(plugin->page());
        addPageToolButton(plugin);
        if (debug)
            qDebug() << "WelcomeMode::initPlugins" << plugin->title();
    }
    QSettings *settings = Core::ICore::instance()->settings();
    const int tabId = settings->value(QLatin1String(currentPageSettingsKeyC), 0).toInt();

    const int pluginCount = ui.stackedWidget->count();
    if (tabId >= 0 && tabId < pluginCount) {
        ui.stackedWidget->setCurrentIndex(tabId);
        if (QToolButton *btn = buttonMap.key(ui.stackedWidget->currentWidget()))
            btn->setChecked(true);
    }
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
        ui.stackedWidget->insertWidget(insertPos, plugin->page());
        addPageToolButton(plugin, insertPos);
        if (debug)
            qDebug() << "welcomePluginAdded" << plugin->title() << "at" << insertPos
                     << " of " << buttonMap.size();
    }
}

void WelcomeMode::showClickedPage()
{
    QToolButton *btn = qobject_cast<QToolButton*>(sender());
    const ToolButtonWidgetMap::const_iterator it = buttonMap.constFind(btn);
    if (it != buttonMap.constEnd())
        ui.stackedWidget->setCurrentWidget(it.value());
}

void WelcomeMode::slotFeedback()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(
        "http://qt.nokia.com/forms/feedback-forms/qt-creator-user-feedback/view")));
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
    addAutoReleasedObject(new Internal::CommunityWelcomePage);

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

} // namespace Welcome
} // namespace Internal


Q_EXPORT_PLUGIN(Welcome::Internal::WelcomePlugin)

#include "welcomeplugin.moc"
