/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "navigatorwidget.h"
#include "navigatorview.h"

#include <designersettings.h>
#include <qmldesignerconstants.h>
#include <qmldesignericons.h>
#include <qmldesignerplugin.h>
#include <theme.h>

#include <QAbstractItemModel>
#include <QBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QStackedWidget>
#include <QToolButton>

#include <utils/fileutils.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

NavigatorWidget::NavigatorWidget(NavigatorView *view)
    : m_treeView(new NavigatorTreeView),
    m_navigatorView(view)
{
    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->setDefaultDropAction(Qt::LinkAction);
    m_treeView->setHeaderHidden(true);

    auto layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);

    auto tabBar = new QTabBar(this);
    tabBar->addTab(tr("Navigator"));
    tabBar->addTab(tr("Project"));
    tabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QWidget *spacer = new QWidget(this);
    spacer->setObjectName(QStringLiteral("itemLibrarySearchInputSpacer"));
    spacer->setFixedHeight(4);

    layout->addWidget(tabBar);
    layout->addWidget(spacer);

    auto stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(m_treeView);

#ifndef QMLDESIGNER_TEST
    auto projectManager = QmlDesignerPlugin::instance()->createProjectExplorerWidget(this);

    QTC_ASSERT(projectManager, ;);
    if (projectManager)
        stackedWidget->addWidget(projectManager);
#endif

    connect(tabBar, &QTabBar::currentChanged, stackedWidget, &QStackedWidget::setCurrentIndex);

    layout->addWidget(stackedWidget);
    setLayout(layout);

    setWindowTitle(tr("Navigator", "Title of navigator view"));

#ifndef QMLDESIGNER_TEST
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));
    m_treeView->setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css"))));
#endif
}

void NavigatorWidget::setTreeModel(QAbstractItemModel* model)
{
    m_treeView->setModel(model);
}

QTreeView *NavigatorWidget::treeView() const
{
    return m_treeView;
}

QList<QToolButton *> NavigatorWidget::createToolBarWidgets()
{
    QList<QToolButton *> buttons;


    auto button = new QToolButton();
    button->setIcon(Icons::ARROW_LEFT.icon());
    button->setToolTip(tr("Become last sibling of parent (CTRL + Left)."));
    button->setShortcut(QKeySequence(Qt::Key_Left | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::leftButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Icons::ARROW_RIGHT.icon());
    button->setToolTip(tr("Become child of last sibling (CTRL + Right)."));
    button->setShortcut(QKeySequence(Qt::Key_Right | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::rightButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Icons::ARROW_DOWN.icon());
    button->setToolTip(tr("Move down (CTRL + Down)."));
    button->setShortcut(QKeySequence(Qt::Key_Down | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::downButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Icons::ARROW_UP.icon());
    button->setToolTip(tr("Move up (CTRL + Up)."));
    button->setShortcut(QKeySequence(Qt::Key_Up | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::upButtonClicked);
    buttons.append(button);

    auto filter = new QToolButton;
    filter->setIcon(Utils::Icons::FILTER.icon());
    filter->setToolTip(tr("Filter Tree"));
    filter->setPopupMode(QToolButton::InstantPopup);
    filter->setProperty("noArrow", true);
    auto filterMenu = new QMenu(filter);
    auto objectAction = new QAction(tr("Show only visible items."), nullptr);
    objectAction->setCheckable(true);
    objectAction->setChecked(
                DesignerSettings::getValue(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS).toBool());
    connect(objectAction, &QAction::toggled, this, &NavigatorWidget::filterToggled);
    filterMenu->addAction(objectAction);
    filter->setMenu(filterMenu);
    buttons.append(filter);

    return buttons;
}

void NavigatorWidget::contextHelpId(const Core::IContext::HelpIdCallback &callback) const
{
    if (navigatorView())
        navigatorView()->contextHelpId(callback);
    else
        callback(QString());
}

void NavigatorWidget::disableNavigator()
{
    m_treeView->setEnabled(false);
}

void NavigatorWidget::enableNavigator()
{
    m_treeView->setEnabled(true);
}


NavigatorView *NavigatorWidget::navigatorView() const
{
    return m_navigatorView.data();
}

}
