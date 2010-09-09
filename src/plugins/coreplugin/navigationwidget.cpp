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

#include "navigationwidget.h"

#include "icore.h"
#include "icontext.h"
#include "coreconstants.h"
#include "inavigationwidgetfactory.h"
#include "modemanager.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/styledbar.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QResizeEvent>
#include <QtGui/QToolButton>
#include <QtGui/QShortcut>
#include <QtGui/QStandardItemModel>

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

NavigationWidget::NavigationWidget(QAction *toggleSideBarAction) :
    m_factoryModel(new QStandardItemModel),
    m_shown(true),
    m_suppressed(false),
    m_width(0),
    m_toggleSideBarAction(toggleSideBarAction)
{
    m_factoryModel->setSortRole(FactoryPriorityRole);
    setOrientation(Qt::Vertical);
    insertSubItem(0, -1); // we don't have any entry to show yet
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

void NavigationWidget::setFactories(const QList<INavigationWidgetFactory *> factories)
{
    ICore *core = ICore::instance();
    ActionManager *am = core->actionManager();
    Context navicontext(Core::Constants::C_NAVIGATION_PANE);

    foreach (INavigationWidgetFactory *factory, factories) {
        const QString id = factory->id();

        QShortcut *shortcut = new QShortcut(this);
        shortcut->setWhatsThis(tr("Activate %1 Pane").arg(factory->displayName()));
        connect(shortcut, SIGNAL(activated()), this, SLOT(activateSubWidget()));
        m_shortcutMap.insert(shortcut, id);

        Core::Command *cmd = am->registerShortcut(shortcut,
            QLatin1String("QtCreator.Sidebar.") + id, navicontext);
        cmd->setDefaultKeySequence(factory->activationSequence());
        m_commandMap.insert(id, cmd);

        QStandardItem *newRow = new QStandardItem(factory->displayName());
        newRow->setData(qVariantFromValue(factory), FactoryObjectRole);
        newRow->setData(factory->id(), FactoryIdRole);
        newRow->setData(factory->priority(), FactoryPriorityRole);
        m_factoryModel->appendRow(newRow);
    }
    m_factoryModel->sort(0);
}

int NavigationWidget::storedWidth()
{
    return m_width;
}

QAbstractItemModel *NavigationWidget::factoryModel() const
{
    return m_factoryModel;
}

void NavigationWidget::updateToggleText()
{
    if (isShown())
        m_toggleSideBarAction->setText(tr("Hide Sidebar"));
    else
        m_toggleSideBarAction->setText(tr("Show Sidebar"));
}

void NavigationWidget::placeHolderChanged(NavigationWidgetPlaceHolder *holder)
{
    m_toggleSideBarAction->setEnabled(holder);
    m_toggleSideBarAction->setChecked(holder && isShown());
    updateToggleText();
}

void NavigationWidget::resizeEvent(QResizeEvent *re)
{
    if (m_width && re->size().width())
        m_width = re->size().width();
    MiniSplitter::resizeEvent(re);
}

NavigationSubWidget *NavigationWidget::insertSubItem(int position,int index)
{
    for (int pos = position + 1; pos < m_subWidgets.size(); ++pos) {
        m_subWidgets.at(pos)->setPosition(pos + 1);
    }

    NavigationSubWidget *nsw = new NavigationSubWidget(this, position, index);
    connect(nsw, SIGNAL(splitMe()), this, SLOT(splitSubWidget()));
    connect(nsw, SIGNAL(closeMe()), this, SLOT(closeSubWidget()));
    insertWidget(position, nsw);
    m_subWidgets.insert(position, nsw);

    return nsw;
}

void NavigationWidget::activateSubWidget()
{
    QShortcut *original = qobject_cast<QShortcut *>(sender());
    QString id = m_shortcutMap[original];
    activateSubWidget(id);
}

void NavigationWidget::activateSubWidget(const QString &factoryId)
{
    setShown(true);
    foreach (NavigationSubWidget *subWidget, m_subWidgets) {
        if (subWidget->factory()->id() == factoryId) {
            subWidget->setFocusWidget();
            return;
        }
    }

    int index = factoryIndex(factoryId);
    if (index >= 0) {
        m_subWidgets.first()->setFactoryIndex(index);
        m_subWidgets.first()->setFocusWidget();
    }
}

void NavigationWidget::splitSubWidget()
{
    NavigationSubWidget *original = qobject_cast<NavigationSubWidget *>(sender());
    int pos = indexOf(original) + 1;
    insertSubItem(pos, original->factoryIndex());
}

void NavigationWidget::closeSubWidget()
{
    if (m_subWidgets.count() != 1) {
        NavigationSubWidget *subWidget = qobject_cast<NavigationSubWidget *>(sender());
        subWidget->saveSettings();
        m_subWidgets.removeOne(subWidget);
        subWidget->hide();
        subWidget->deleteLater();
    } else {
        setShown(false);
    }
}

void NavigationWidget::saveSettings(QSettings *settings)
{
    QStringList viewIds;
    for (int i=0; i<m_subWidgets.count(); ++i) {
        m_subWidgets.at(i)->saveSettings();
        viewIds.append(m_subWidgets.at(i)->factory()->id());
    }
    settings->setValue("Navigation/Views", viewIds);
    settings->setValue("Navigation/Visible", isShown());
    settings->setValue("Navigation/VerticalPosition", saveState());
    settings->setValue("Navigation/Width", m_width);
}

void NavigationWidget::restoreSettings(QSettings *settings)
{
    int version = settings->value("Navigation/Version", 1).toInt();
    QStringList viewIds = settings->value("Navigation/Views").toStringList();

    bool restoreSplitterState = true;
    if (version == 1) {
        if (viewIds.isEmpty())
            viewIds += "Projects";
        if (!viewIds.contains("Open Documents")) {
            viewIds += "Open Documents";
            restoreSplitterState = false;
        }
        settings->setValue("Navigation/Version", 2);
    }

    for (int i=0; i<viewIds.count(); ++i) {
        const QString &view = viewIds.at(i);
        int index = factoryIndex(view);

        if (i >= m_subWidgets.size()) {
            insertSubItem(i, index);
        } else {
            m_subWidgets.at(i)->setFactoryIndex(index);
        }
    }

    if (settings->contains("Navigation/Visible")) {
        setShown(settings->value("Navigation/Visible").toBool());
    } else {
        setShown(true);
    }

    if (restoreSplitterState && settings->contains("Navigation/VerticalPosition")) {
        restoreState(settings->value("Navigation/VerticalPosition").toByteArray());
    } else {
        QList<int> sizes;
        sizes += 256;
        for (int i = viewIds.size()-1; i > 0; --i)
            sizes.prepend(512);
        setSizes(sizes);
    }

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
}

void NavigationWidget::closeSubWidgets()
{
    foreach (NavigationSubWidget *subWidget, m_subWidgets) {
        subWidget->saveSettings();
        delete subWidget;
    }
    m_subWidgets.clear();
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
    updateToggleText();
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

int NavigationWidget::factoryIndex(const QString &id)
{
    for (int row = 0; row < m_factoryModel->rowCount(); ++row) {
        if (m_factoryModel->data(m_factoryModel->index(row, 0), FactoryIdRole).toString() == id) {
            return row;
        }
    }
    return -1;
}

////
// NavigationSubWidget
////


NavigationSubWidget::NavigationSubWidget(NavigationWidget *parentWidget, int position, int factoryIndex)
    : m_parentWidget(parentWidget),
      m_position(position)
{
    m_navigationComboBox = new NavComboBox(this);
    m_navigationComboBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_navigationComboBox->setFocusPolicy(Qt::TabFocus);
    m_navigationComboBox->setMinimumContentsLength(0);
    m_navigationComboBox->setModel(parentWidget->factoryModel());
    m_navigationWidget = 0;
    m_navigationWidgetFactory = 0;

    m_toolBar = new Utils::StyledBar(this);
    QHBoxLayout *toolBarLayout = new QHBoxLayout;
    toolBarLayout->setMargin(0);
    toolBarLayout->setSpacing(0);
    m_toolBar->setLayout(toolBarLayout);
    toolBarLayout->addWidget(m_navigationComboBox);

    QToolButton *splitAction = new QToolButton();
    splitAction->setIcon(QIcon(QLatin1String(Constants::ICON_SPLIT_HORIZONTAL)));
    splitAction->setToolTip(tr("Split"));
    QToolButton *close = new QToolButton();
    close->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE)));
    close->setToolTip(tr("Close"));

    toolBarLayout->addWidget(splitAction);
    toolBarLayout->addWidget(close);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setMargin(0);
    lay->setSpacing(0);
    setLayout(lay);
    lay->addWidget(m_toolBar);

    connect(splitAction, SIGNAL(clicked()), this, SIGNAL(splitMe()));
    connect(close, SIGNAL(clicked()), this, SIGNAL(closeMe()));

    setFactoryIndex(factoryIndex);

    connect(m_navigationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(comboBoxIndexChanged(int)));

    comboBoxIndexChanged(factoryIndex);
}

NavigationSubWidget::~NavigationSubWidget()
{
}

void NavigationSubWidget::comboBoxIndexChanged(int factoryIndex)
{
    saveSettings();

    // Remove toolbutton
    foreach (QWidget *w, m_additionalToolBarWidgets)
        delete w;
    m_additionalToolBarWidgets.clear();

    // Remove old Widget
    delete m_navigationWidget;
    m_navigationWidget = 0;
    m_navigationWidgetFactory = 0;
    if (factoryIndex == -1)
        return;

    // Get new stuff
    m_navigationWidgetFactory = m_navigationComboBox->itemData(factoryIndex,
                           NavigationWidget::FactoryObjectRole).value<INavigationWidgetFactory *>();
    NavigationView n = m_navigationWidgetFactory->createWidget();
    m_navigationWidget = n.widget;
    layout()->addWidget(m_navigationWidget);

    // Add Toolbutton
    m_additionalToolBarWidgets = n.dockToolBarWidgets;
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(m_toolBar->layout());
    foreach (QToolButton *w, m_additionalToolBarWidgets) {
        layout->insertWidget(layout->count()-2, w);
    }

    restoreSettings();
}

void NavigationSubWidget::setFocusWidget()
{
    if (m_navigationWidget)
        m_navigationWidget->setFocus();
}

INavigationWidgetFactory *NavigationSubWidget::factory()
{
    return m_navigationWidgetFactory;
}


void NavigationSubWidget::saveSettings()
{
    if (!m_navigationWidget || !factory())
        return;
    factory()->saveSettings(position(), m_navigationWidget);
}

void NavigationSubWidget::restoreSettings()
{
    if (!m_navigationWidget || !factory())
        return;
    factory()->restoreSettings(position(), m_navigationWidget);
}

Core::Command *NavigationSubWidget::command(const QString &title) const
{
    const QHash<QString, Core::Command*> commandMap = m_parentWidget->commandMap();
    QHash<QString, Core::Command*>::const_iterator r = commandMap.find(title);
    if (r != commandMap.end())
        return r.value();
    return 0;
}

int NavigationSubWidget::factoryIndex() const
{
    return m_navigationComboBox->currentIndex();
}

void NavigationSubWidget::setFactoryIndex(int i)
{
    m_navigationComboBox->setCurrentIndex(i);
}

int NavigationSubWidget::position() const
{
    return m_position;
}

void NavigationSubWidget::setPosition(int position)
{
    m_position = position;
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
