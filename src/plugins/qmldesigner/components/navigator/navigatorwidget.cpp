// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorsearchwidget.h"
#include "navigatorwidget.h"
#include "navigatorview.h"

#include <designeractionmanager.h>
#include <designersettings.h>
#include <theme.h>
#include <qmldesignerconstants.h>
#include <qmldesignericons.h>
#include <qmldesignerplugin.h>

#include <QAbstractItemModel>
#include <QBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>

#include <utils/fileutils.h>
#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

NavigatorWidget::NavigatorWidget(NavigatorView *view)
    : m_treeView(new NavigatorTreeView)
    , m_navigatorView(view)
{
    setAcceptDrops(true);

    m_treeView->setDragEnabled(true);
    m_treeView->setAcceptDrops(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->setDefaultDropAction(Qt::LinkAction);
    m_treeView->setHeaderHidden(true);

    auto layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QWidget *toolBar = createToolBar();

    toolBar->setParent(this);
    layout->addWidget(toolBar);

    m_searchWidget = new NavigatorSearchWidget();
    connect(m_searchWidget, &NavigatorSearchWidget::textChanged, this, &NavigatorWidget::textFilterChanged);
    layout->addWidget(m_searchWidget);

    layout->addWidget(m_treeView);
    setLayout(layout);

    setWindowTitle(tr("Navigator", "Title of navigator view"));

    QByteArray sheet = Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css");
    sheet += Utils::FileReader::fetchQrc(":/qmldesigner/scrollbar.css");
    setStyleSheet(Theme::replaceCssColors(QString::fromUtf8(sheet)));

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_NAVIGATORVIEW_TIME);
}

void NavigatorWidget::setTreeModel(QAbstractItemModel *model)
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
    auto filterAction = new QAction(tr("Show Only Visible Components"), nullptr);
    filterAction->setCheckable(true);

    bool filterFlag = QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS).toBool();
    filterAction->setChecked(filterFlag);

    connect(filterAction, &QAction::toggled, this, &NavigatorWidget::filterToggled);
    filterMenu->addAction(filterAction);

    auto reverseAction = new QAction(tr("Reverse Component Order"), nullptr);
    reverseAction->setCheckable(true);

    bool reverseFlag = QmlDesignerPlugin::settings().value(DesignerSettingsKey::NAVIGATOR_REVERSE_ITEM_ORDER).toBool();
    reverseAction->setChecked(reverseFlag);

    connect(reverseAction, &QAction::toggled, this, &NavigatorWidget::reverseOrderToggled);
    filterMenu->addAction(reverseAction);

    filter->setMenu(filterMenu);
    buttons.append(filter);

    return buttons;
}

QToolBar *NavigatorWidget::createToolBar()
{
    const QList<QToolButton*> buttons = createToolBarWidgets();

    auto toolBar = new QToolBar();
    for (auto toolButton : buttons)
        toolBar->addWidget(toolButton);

    return toolBar;
}

void NavigatorWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (auto view = navigatorView())
        QmlDesignerPlugin::contextHelp(callback, view->contextHelpId());
    else
        callback({});
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

void NavigatorWidget::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData()))
        dragEnterEvent->acceptProposedAction();
}

void NavigatorWidget::dropEvent(QDropEvent *dropEvent)
{
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    actionManager.handleExternalAssetsDrop(dropEvent->mimeData());
}

void NavigatorWidget::setDragType(const QByteArray &type)
{
    m_dragType = type;
}

QByteArray NavigatorWidget::dragType() const
{
    return m_dragType;
}

void NavigatorWidget::clearSearch()
{
    m_searchWidget->clear();
}

}
