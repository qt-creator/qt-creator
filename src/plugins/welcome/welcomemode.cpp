/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "welcomemode.h"
#include "ui_welcomemode.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>

#include <utils/styledbar.h>
#include <utils/welcomemodetreewidget.h>
#include <utils/iwelcomepage.h>

#include <QtGui/QScrollArea>
#include <QtGui/QDesktopServices>
#include <QtGui/QToolButton>

#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include <QtCore/QUrl>

enum { debug = 0 };

using namespace ExtensionSystem;

static const char currentPageSettingsKeyC[] = "General/WelcomeTab";

namespace Welcome {

struct WelcomeModePrivate
{
    typedef QMap<QToolButton*, QWidget*> ToolButtonWidgetMap;

    WelcomeModePrivate() {}

    QScrollArea *m_scrollArea;
    QWidget *m_widget;
    QWidget *m_welcomePage;
    ToolButtonWidgetMap buttonMap;
    QHBoxLayout * buttonLayout;
    Ui::WelcomeMode ui;
};

// ---  WelcomeMode
WelcomeMode::WelcomeMode() :
    m_d(new WelcomeModePrivate)
{
    m_d->m_widget = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(m_d->m_widget);
    l->setMargin(0);
    l->setSpacing(0);
    l->addWidget(new Utils::StyledBar(m_d->m_widget));
    m_d->m_welcomePage = new QWidget(m_d->m_widget);
    m_d->ui.setupUi(m_d->m_welcomePage);
    m_d->ui.helpUsLabel->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    m_d->ui.feedbackButton->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    l->addWidget(m_d->m_welcomePage);

    m_d->m_scrollArea = new QScrollArea;
    m_d->m_scrollArea->setFrameStyle(QFrame::NoFrame);
    m_d->m_scrollArea->setWidget(m_d->m_widget);
    m_d->m_scrollArea->setWidgetResizable(true);

    PluginManager *pluginManager = PluginManager::instance();
    connect(pluginManager, SIGNAL(objectAdded(QObject*)), SLOT(welcomePluginAdded(QObject*)));

    connect(m_d->ui.feedbackButton, SIGNAL(clicked()), SLOT(slotFeedback()));
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->setValue(QLatin1String(currentPageSettingsKeyC), m_d->ui.stackedWidget->currentIndex());
    delete m_d->m_widget;
    delete m_d;
}

QString WelcomeMode::displayName() const
{
    return tr("Welcome");
}

QIcon WelcomeMode::icon() const
{
    return QIcon(QLatin1String(":/core/images/qtcreator_logo_32.png"));
}

int WelcomeMode::priority() const
{
    return Core::Constants::P_MODE_WELCOME;
}

QWidget* WelcomeMode::widget()
{
    return m_d->m_scrollArea;
}

QString WelcomeMode::id() const
{
    return QLatin1String(Core::Constants::MODE_WELCOME);
}

QList<int> WelcomeMode::context() const
{
    static QList<int> contexts = QList<int>()
                                 << Core::UniqueIDManager::instance()->uniqueIdentifier(Core::Constants::C_WELCOME_MODE);
    return contexts;
}

bool sortFunction(Utils::IWelcomePage * a, Utils::IWelcomePage *b)
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
    m_d->buttonMap.insert(btn, plugin->page());
    if (position >= 0) {
        m_d->buttonLayout->insertWidget(position, btn);
    } else {
        m_d->buttonLayout->addWidget(btn);
    }
    return btn;
}

void WelcomeMode::initPlugins()
{
    m_d->buttonLayout = new QHBoxLayout(m_d->ui.navFrame);
    m_d->buttonLayout->setMargin(0);
    m_d->buttonLayout->setSpacing(0);
    QList<Utils::IWelcomePage*> plugins = PluginManager::instance()->getObjects<Utils::IWelcomePage>();
    qSort(plugins.begin(), plugins.end(), &sortFunction);
    foreach (Utils::IWelcomePage *plugin, plugins) {
        m_d->ui.stackedWidget->addWidget(plugin->page());
        addPageToolButton(plugin);
        if (debug)
            qDebug() << "WelcomeMode::initPlugins" << plugin->title();
    }
    m_d->buttonLayout->addSpacing(5);

    QSettings *settings = Core::ICore::instance()->settings();
    const int tabId = settings->value(QLatin1String(currentPageSettingsKeyC), 0).toInt();

    const int pluginCount = m_d->ui.stackedWidget->count();
    if (tabId >= 0 && tabId < pluginCount) {
        m_d->ui.stackedWidget->setCurrentIndex(tabId);
        if (QToolButton *btn = m_d->buttonMap.key(m_d->ui.stackedWidget->currentWidget()))
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
        m_d->ui.stackedWidget->insertWidget(insertPos, plugin->page());
        addPageToolButton(plugin, insertPos);
        if (debug)
            qDebug() << "welcomePluginAdded" << plugin->title() << "at" << insertPos
                     << " of " << m_d->buttonMap.size();
    }
}

void WelcomeMode::showClickedPage()
{
    QToolButton *btn = qobject_cast<QToolButton*>(sender());
    const WelcomeModePrivate::ToolButtonWidgetMap::const_iterator it = m_d->buttonMap.constFind(btn);
    if (it != m_d->buttonMap.constEnd())
        m_d->ui.stackedWidget->setCurrentWidget(it.value());
}

void WelcomeMode::slotFeedback()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(
        "http://qt.nokia.com/forms/feedback-forms/qt-creator-user-feedback/view")));
}

} // namespace Welcome
