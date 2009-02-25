/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "navigationwidget.h"

#include "icore.h"
#include "coreconstants.h"
#include "inavigationwidgetfactory.h"
#include "modemanager.h"
#include "uniqueidmanager.h"
#include "actionmanager/actionmanager.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QResizeEvent>
#include <QtGui/QToolBar>
#include <QtGui/QToolButton>

Q_DECLARE_METATYPE(Core::INavigationWidgetFactory *)

using namespace Core;
using namespace Core::Internal;

NavigationWidgetPlaceHolder *NavigationWidgetPlaceHolder::m_current = 0;

NavigationWidgetPlaceHolder* NavigationWidgetPlaceHolder::current()
{
    return m_current;
}

NavigationWidgetPlaceHolder::NavigationWidgetPlaceHolder(Core::IMode *mode, QWidget *parent)
    :QWidget(parent), m_mode(mode)
{
    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    connect(Core::ModeManager::instance(), SIGNAL(currentModeAboutToChange(Core::IMode *)),
            this, SLOT(currentModeAboutToChange(Core::IMode *)));
}

NavigationWidgetPlaceHolder::~NavigationWidgetPlaceHolder()
{
    if (m_current == this) {
        NavigationWidget::instance()->setParent(0);
        NavigationWidget::instance()->hide();
    }
}

void NavigationWidgetPlaceHolder::applyStoredSize(int width)
{
    if (width) {
        QSplitter *splitter = qobject_cast<QSplitter *>(parentWidget());
        if (splitter) {
            // A splitter we need to resize the splitter sizes
            QList<int> sizes = splitter->sizes();
            int index = splitter->indexOf(this);
            int diff = width - sizes.at(index);
            int adjust = sizes.count() > 1 ? (diff / (sizes.count() - 1)) : 0;
            for (int i = 0; i < sizes.count(); ++i) {
                if (i != index)
                    sizes[i] += adjust;
            }
            sizes[index]= width;
            splitter->setSizes(sizes);
        } else {
            QSize s = size();
            s.setWidth(width);
            resize(s);
        }
    }
}

// This function does work even though the order in which
// the placeHolder get the signal is undefined.
// It does ensure that after all PlaceHolders got the signal
// m_current points to the current PlaceHolder, or zero if there
// is no PlaceHolder in this mode
// And that the parent of the NavigationWidget gets the correct parent
void NavigationWidgetPlaceHolder::currentModeAboutToChange(Core::IMode *mode)
{
    NavigationWidget *navigationWidget = NavigationWidget::instance();

    if (m_current == this) {
        m_current = 0;
        navigationWidget->setParent(0);
        navigationWidget->hide();
        navigationWidget->placeHolderChanged(m_current);
    }
    if (m_mode == mode) {
        m_current = this;

        int width = navigationWidget->storedWidth();

        layout()->addWidget(navigationWidget);
        navigationWidget->show();

        applyStoredSize(width);
        setVisible(navigationWidget->isShown());
        navigationWidget->placeHolderChanged(m_current);
    }
}

NavigationWidget *NavigationWidget::m_instance = 0;

NavigationWidget::NavigationWidget(QAction *toggleSideBarAction)
    : m_shown(true),
      m_suppressed(false),
      m_width(0),
      m_toggleSideBarAction(toggleSideBarAction)
{
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),
            this, SLOT(objectAdded(QObject*)));

    setOrientation(Qt::Vertical);
    insertSubItem(0);
    m_instance = this;
}

NavigationWidget::~NavigationWidget()
{
    m_instance = 0;
}

NavigationWidget *NavigationWidget::instance()
{
    return m_instance;
}

int NavigationWidget::storedWidth()
{
    return m_width;
}

void NavigationWidget::placeHolderChanged(NavigationWidgetPlaceHolder *holder)
{
    m_toggleSideBarAction->setEnabled(holder);
    m_toggleSideBarAction->setChecked(holder && isShown());
}

void NavigationWidget::resizeEvent(QResizeEvent *re)
{
    if (m_width && re->size().width())
        m_width = re->size().width();
    MiniSplitter::resizeEvent(re);
}

NavigationSubWidget *NavigationWidget::insertSubItem(int position)
{
    NavigationSubWidget *nsw = new NavigationSubWidget(this);
    connect(nsw, SIGNAL(split()), this, SLOT(split()));
    connect(nsw, SIGNAL(close()), this, SLOT(close()));
    insertWidget(position, nsw);
    m_subWidgets.insert(position, nsw);
    return nsw;
}

void NavigationWidget::activateSubWidget()
{
    QShortcut *original = qobject_cast<QShortcut *>(sender());
    QString title = m_shortcutMap[original];

    foreach (NavigationSubWidget *subWidget, m_subWidgets)
        if (subWidget->factory()->displayName() == title) {
            subWidget->setFocusWidget();
            return;
        }

    m_subWidgets.first()->setFactory(title);
    m_subWidgets.first()->setFocusWidget();
}

void NavigationWidget::split()
{
    NavigationSubWidget *original = qobject_cast<NavigationSubWidget *>(sender());
    int pos = indexOf(original) + 1;
    NavigationSubWidget *newnsw = insertSubItem(pos);
    newnsw->setFactory(original->factory());
}

void NavigationWidget::close()
{
    if (m_subWidgets.count() != 1) {
        NavigationSubWidget *subWidget = qobject_cast<NavigationSubWidget *>(sender());
        m_subWidgets.removeOne(subWidget);
        subWidget->hide();
        subWidget->deleteLater();
    } else {
        setShown(false);
    }
}



void NavigationWidget::saveSettings(QSettings *settings)
{
    QStringList views;
    for (int i=0; i<m_subWidgets.count(); ++i) {
        views.append(m_subWidgets.at(i)->factory()->displayName());
    }
    settings->setValue("Navigation/Views", views);
    settings->setValue("Navigation/Visible", isShown());
    settings->setValue("Navigation/VerticalPosition", saveState());
    settings->setValue("Navigation/Width", m_width);

    for (int i=0; i<m_subWidgets.count(); ++i)
        m_subWidgets.at(i)->saveSettings(i);
}

void NavigationWidget::restoreSettings(QSettings *settings)
{
    if (settings->contains("Navigation/Views")) {
        QStringList views = settings->value("Navigation/Views").toStringList();
        for (int i=0; i<views.count()-1; ++i) {
            insertSubItem(0);
        }
        for (int i=0; i<views.count(); ++i) {
            const QString &view = views.at(i);
            NavigationSubWidget *nsw = m_subWidgets.at(i);
            nsw->setFactory(view);
        }
    }

    if (settings->contains("Navigation/Visible")) {
        setShown(settings->value("Navigation/Visible").toBool());
    } else {
        setShown(true);
    }

    if (settings->contains("Navigation/VerticalPosition"))
        restoreState(settings->value("Navigation/VerticalPosition").toByteArray());

    if (settings->contains("Navigation/Width")) {
        m_width = settings->value("Navigation/Width").toInt();
        if (!m_width)
            m_width = 240;
    } else {
        m_width = 240; //pixel
    }
    // Apply
    if (NavigationWidgetPlaceHolder::m_current) {
        NavigationWidgetPlaceHolder::m_current->applyStoredSize(m_width);
    }

    for (int i=0; i<m_subWidgets.count(); ++i)
        m_subWidgets.at(i)->restoreSettings(i);
}

void NavigationWidget::setShown(bool b)
{
    if (m_shown == b)
        return;
    m_shown = b;
    if (NavigationWidgetPlaceHolder::m_current) {
        NavigationWidgetPlaceHolder::m_current->setVisible(m_shown && !m_suppressed);
        m_toggleSideBarAction->setChecked(m_shown);
    } else {
        m_toggleSideBarAction->setChecked(false);
    }
}

bool NavigationWidget::isShown() const
{
    return m_shown;
}

bool NavigationWidget::isSuppressed() const
{
    return m_suppressed;
}

void NavigationWidget::setSuppressed(bool b)
{
    if (m_suppressed == b)
        return;
    m_suppressed = b;
    if (NavigationWidgetPlaceHolder::m_current)
        NavigationWidgetPlaceHolder::m_current->setVisible(m_shown && !m_suppressed);
}

void NavigationWidget::objectAdded(QObject * obj)
{
    INavigationWidgetFactory *factory = Aggregation::query<INavigationWidgetFactory>(obj);
    if (!factory)
        return;

    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    QList<int> navicontext = QList<int>() << core->uniqueIDManager()->
        uniqueIdentifier(Core::Constants::C_NAVIGATION_PANE);

    QString displayName = factory->displayName();
    QShortcut *shortcut = new QShortcut(this);
    shortcut->setWhatsThis(tr("Activate %1 Pane").arg(displayName));
    Core::Command *cmd = am->registerShortcut(shortcut,
        QLatin1String("QtCreator.Sidebar.") + displayName, navicontext);
    cmd->setDefaultKeySequence(factory->activationSequence());
    connect(shortcut, SIGNAL(activated()), this, SLOT(activateSubWidget()));

    m_shortcutMap.insert(shortcut, displayName);
    m_commandMap.insert(displayName, cmd);
}

////
// NavigationSubWidget
////


NavigationSubWidget::NavigationSubWidget(NavigationWidget *parentWidget)
    : m_parentWidget(parentWidget)
{
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(objectAdded(QObject*)),
            this, SLOT(objectAdded(QObject*)));
    connect(ExtensionSystem::PluginManager::instance(), SIGNAL(aboutToRemoveObject(QObject*)),
            this, SLOT(aboutToRemoveObject(QObject*)));

    m_navigationComboBox = new NavComboBox(this);
    m_navigationWidget = 0;
#ifdef Q_OS_MAC
    // this is to avoid ugly tool bar behavior
    m_navigationComboBox->setMaximumWidth(130);
#endif

    m_toolBar = new QToolBar(this);
    m_toolBar->setContentsMargins(0, 0, 0, 0);
    m_toolBar->addWidget(m_navigationComboBox);

    m_splitAction = new QAction(QIcon(":/core/images/splitbutton_horizontal.png"), tr("Split"), this);
    QAction *close = new QAction(QIcon(":/core/images/closebutton.png"), tr("Close"), this);

    QWidget *spacerItem = new QWidget(this);
    spacerItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_toolBar->addWidget(spacerItem);
    m_toolBar->addAction(m_splitAction);
    m_toolBar->addAction(close);

    m_toolBar->widgetForAction(m_splitAction)->setProperty("type", QLatin1String("dockbutton"));
    m_toolBar->widgetForAction(close)->setProperty("type", QLatin1String("dockbutton"));

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setMargin(0);
    lay->setSpacing(0);
    setLayout(lay);
    lay->addWidget(m_toolBar);

    connect(m_splitAction, SIGNAL(triggered()), this, SIGNAL(split()));
    connect(close, SIGNAL(triggered()), this, SIGNAL(close()));
    connect(m_navigationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setCurrentIndex(int)));

    foreach (INavigationWidgetFactory *factory, ExtensionSystem::PluginManager::instance()->getObjects<INavigationWidgetFactory>()) {
        QVariant v;
        v.setValue(factory);
        m_navigationComboBox->addItem(factory->displayName(), v);
    }
}

NavigationSubWidget::~NavigationSubWidget()
{
}

void NavigationSubWidget::setCurrentIndex(int index)
{
    // Remove toolbutton
    foreach (QWidget *w, m_additionalToolBarWidgets)
        delete w;

    // Remove old Widget
    delete m_navigationWidget;
    if (index == -1)
        return;

    // Get new stuff
    INavigationWidgetFactory *factory = m_navigationComboBox->itemData(index).value<INavigationWidgetFactory *>();
    NavigationView n = factory->createWidget();
    m_navigationWidget = n.widget;
    layout()->addWidget(m_navigationWidget);

    // Add Toolbutton
    m_additionalToolBarWidgets = n.doockToolBarWidgets;
    foreach (QToolButton *w, m_additionalToolBarWidgets) {
        m_toolBar->insertWidget(m_splitAction, w);
    }
}

void NavigationSubWidget::setFocusWidget()
{
    if (m_navigationWidget)
        m_navigationWidget->setFocus();
}

void NavigationSubWidget::objectAdded(QObject * obj)
{
    INavigationWidgetFactory *factory = Aggregation::query<INavigationWidgetFactory>(obj);
    if (!factory)
        return;

    QVariant v;
    v.setValue(factory);
    m_navigationComboBox->addItem(factory->displayName(), v);
    //qDebug()<<"added factory :"<<factory<<m_navigationComboBox->findData(v);
}

void NavigationSubWidget::aboutToRemoveObject(QObject *obj)
{
    INavigationWidgetFactory *factory = Aggregation::query<INavigationWidgetFactory>(obj);
    if (!factory)
        return;
    QVariant v;
    v.setValue(factory);
    int index = m_navigationComboBox->findData(v);
    if (index == -1) {
        qDebug()<<"factory not found not removing" << factory;
        return;
    }
    m_navigationComboBox->removeItem(index);
}

void NavigationSubWidget::setFactory(INavigationWidgetFactory *factory)
{
    QVariant v;
    v.setValue(factory);
    int index = m_navigationComboBox->findData(v);
    if (index == -1)
        return;
    m_navigationComboBox->setCurrentIndex(index);
}

void NavigationSubWidget::setFactory(const QString &name)
{
    for (int i = 0; i < m_navigationComboBox->count(); ++i) {
        INavigationWidgetFactory *factory =
                m_navigationComboBox->itemData(i).value<INavigationWidgetFactory *>();
        if (factory->displayName() == name)
            m_navigationComboBox->setCurrentIndex(i);
    }
}

INavigationWidgetFactory *NavigationSubWidget::factory()
{
    int index = m_navigationComboBox->currentIndex();
    if (index == -1)
        return 0;
    return m_navigationComboBox->itemData(index).value<INavigationWidgetFactory *>();
}

void NavigationSubWidget::saveSettings(int position)
{
    factory()->saveSettings(position, m_navigationWidget);
}

void NavigationSubWidget::restoreSettings(int position)
{
    factory()->restoreSettings(position, m_navigationWidget);
}

Core::Command *NavigationSubWidget::command(const QString &title) const
{
    const QHash<QString, Core::Command*> commandMap = m_parentWidget->commandMap();
    QHash<QString, Core::Command*>::const_iterator r = commandMap.find(title);
    if (r != commandMap.end())
        return r.value();
    return 0;
}

NavComboBox::NavComboBox(NavigationSubWidget *navSubWidget)
    : m_navSubWidget(navSubWidget)
{
}

bool NavComboBox::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        QString txt = currentText();
        Core::Command *cmd = m_navSubWidget->command(txt);
        if (cmd) {
            txt = tr("Activate %1").arg(txt);
            setToolTip(cmd->stringWithAppendedShortcut(txt));
        } else {
            setToolTip(txt);
        }
    }
    return QComboBox::event(e);
}
