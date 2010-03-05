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
#include <extensionsystem/pluginmanager.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/dialogs/newdialog.h>

#include <utils/styledbar.h>
#include <utils/welcomemodetreewidget.h>
#include <utils/iwelcomepage.h>

#include <QtGui/QMouseEvent>
#include <QtGui/QScrollArea>
#include <QtGui/QButtonGroup>
#include <QtGui/QDesktopServices>
#include <QtGui/QToolButton>

#include <QtCore/QSettings>
#include <QtCore/QUrl>
#include <QtCore/QDebug>

#include <cstdlib>

#include "ui_welcomemode.h"

using namespace ExtensionSystem;
using namespace Utils;

namespace Welcome {

struct WelcomeModePrivate
{
    WelcomeModePrivate();

    QScrollArea *m_scrollArea;
    QWidget *m_widget;
    QWidget *m_welcomePage;
    QMap<QAbstractButton*, QWidget*> buttonMap;
    QHBoxLayout * buttonLayout;
    Ui::WelcomeMode ui;
    int currentTip;
};

WelcomeModePrivate::WelcomeModePrivate()
{
}

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
    settings->setValue("General/WelcomeTab", m_d->ui.stackedWidget->currentIndex());
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

bool sortFunction(IWelcomePage * a, IWelcomePage *b)
{
    return a->priority() < b->priority();
}

void WelcomeMode::initPlugins()
{
    m_d->buttonLayout = new QHBoxLayout(m_d->ui.navFrame);
    m_d->buttonLayout->setMargin(0);
    m_d->buttonLayout->setSpacing(0);
    delete m_d->ui.stackedWidget->currentWidget();
    QList<IWelcomePage*> plugins = PluginManager::instance()->getObjects<IWelcomePage>();
    qSort(plugins.begin(), plugins.end(), &sortFunction);
    foreach (IWelcomePage* plugin, plugins) {
        QToolButton * btn = new QToolButton;
        btn->setCheckable(true);
        btn->setText(plugin->title());
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        btn->setAutoExclusive(true);
        connect (btn, SIGNAL(clicked()), SLOT(showClickedPage()));
        m_d->ui.stackedWidget->addWidget(plugin->page());
        m_d->buttonLayout->addWidget(btn);
        m_d->buttonMap.insert(btn, plugin->page());
    }
    m_d->buttonLayout->addSpacing(5);

    QSettings *settings = Core::ICore::instance()->settings();
    int tabId = settings->value("General/WelcomeTab", 0).toInt();

    int pluginCount = m_d->ui.stackedWidget->count();
    if (tabId < pluginCount) {
        m_d->ui.stackedWidget->setCurrentIndex(tabId);
        QMapIterator<QAbstractButton*, QWidget*> it(m_d->buttonMap);
        while (it.hasNext())
            if (it.next().value() == m_d->ui.stackedWidget->currentWidget()) {
                it.key()->setChecked(true);
                break;
            }
    }

}

void WelcomeMode::welcomePluginAdded(QObject *obj)
{
    if (IWelcomePage *plugin = qobject_cast<IWelcomePage*>(obj))
    {
        QToolButton * btn = new QToolButton;
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setText(plugin->title());
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect (btn, SIGNAL(clicked()), SLOT(showClickedPage()));
        int insertPos = 0;
        QList<IWelcomePage*> plugins = PluginManager::instance()->getObjects<IWelcomePage>();
        foreach (IWelcomePage* p, plugins) {
            if (plugin->priority() < p->priority())
                insertPos++;
            else
                break;
        }
        m_d->ui.stackedWidget->insertWidget(insertPos, plugin->page());
        m_d->buttonMap.insert(btn, plugin->page());
        m_d->buttonLayout->insertWidget(insertPos, btn);
    }
}

void WelcomeMode::showClickedPage()
{
    QAbstractButton *btn = qobject_cast<QAbstractButton*>(sender());
    QMap<QAbstractButton*, QWidget*>::iterator it = m_d->buttonMap.find(btn);
    if (it.value())
        m_d->ui.stackedWidget->setCurrentWidget(it.value());
}

void WelcomeMode::slotFeedback()
{
    QDesktopServices::openUrl(QUrl(QLatin1String(
        "http://qt.nokia.com/forms/feedback-forms/qt-creator-user-feedback/view")));
}


} // namespace Welcome
