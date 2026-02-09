// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "navigatorwidget.h"
#include "navigatorsearchwidget.h"
#include "navigatortracing.h"
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
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

using namespace Core;

namespace QmlDesigner {

using NavigatorTracing::category;

NavigatorWidget::NavigatorWidget(NavigatorView *view)
    : m_treeView(new NavigatorTreeView)
    , m_navigatorView(view)
{
    NanotraceHR::Tracer tracer{"navigator widget constructor", category()};

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

    m_searchWidget = new NavigatorSearchWidget();
    connect(m_searchWidget,
            &NavigatorSearchWidget::textChanged,
            this,
            &NavigatorWidget::textFilterChanged);
    layout->addWidget(m_searchWidget);

    QWidget *toolBar = createToolBar();
    toolBar->setParent(this);
    layout->addWidget(toolBar);

    layout->addWidget(m_treeView);
    setLayout(layout);

    setWindowTitle(tr("Navigator", "Title of navigator view"));

    QString sheet = Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css");
    setStyleSheet(Theme::replaceCssColors(sheet));

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_NAVIGATORVIEW_TIME);

    setFocusProxy(m_treeView);

    IContext::attach(this,
                     Core::Context(Constants::qmlNavigatorContextId, Constants::qtQuickToolsMenuContextId),
                     [this](const IContext::HelpCallback &callback) { contextHelp(callback); });
}

void NavigatorWidget::setTreeModel(QAbstractItemModel *model)
{
    NanotraceHR::Tracer tracer{"navigator widget set tree model", category()};

    m_treeView->setModel(model);
}

QTreeView *NavigatorWidget::treeView() const
{
    NanotraceHR::Tracer tracer{"navigator widget tree view", category()};

    return m_treeView;
}

QList<QWidget *> NavigatorWidget::createToolBarWidgets()
{
    NanotraceHR::Tracer tracer{"navigator widget create toolbar widgets", category()};

    QList<QWidget *> buttons;

    auto empty = new QWidget();
    empty->setFixedWidth(5);
    empty->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    buttons.append(empty);

    auto button = new QToolButton();
    button->setIcon(Theme::iconFromName(Theme::Icon::moveUpwards_medium));
    button->setToolTip(tr("Become last sibling of parent (CTRL + Left)."));
    button->setShortcut(QKeySequence(Qt::Key_Left | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::leftButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Theme::iconFromName(Theme::Icon::moveInwards_medium));
    button->setToolTip(tr("Become child of last sibling (CTRL + Right)."));
    button->setShortcut(QKeySequence(Qt::Key_Right | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::rightButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Theme::iconFromName(Theme::Icon::moveDown_medium));
    button->setToolTip(tr("Move down (CTRL + Down)."));
    button->setShortcut(QKeySequence(Qt::Key_Down | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::downButtonClicked);
    buttons.append(button);

    button = new QToolButton();
    button->setIcon(Theme::iconFromName(Theme::Icon::moveUp_medium));
    button->setToolTip(tr("Move up (CTRL + Up)."));
    button->setShortcut(QKeySequence(Qt::Key_Up | Qt::CTRL));
    connect(button, &QAbstractButton::clicked, this, &NavigatorWidget::upButtonClicked);
    buttons.append(button);

    empty = new QWidget();
    empty->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    buttons.append(empty);

    // Colorize Component Icons
    const QIcon colorizeOffIcon = Theme::iconFromName(Theme::Icon::colorSelection_medium);
    const QIcon colorizeOnIcon = Theme::iconFromName(Theme::Icon::colorSelection_medium,
                                             Theme::getColor(Theme::Color::DStextSelectedTextColor));
    QIcon cIcon;
    cIcon.addPixmap(colorizeOnIcon.pixmap({16, 16}), QIcon::Normal, QIcon::On);
    cIcon.addPixmap(colorizeOffIcon.pixmap({16, 16}), QIcon::Normal, QIcon::Off);

    button = new QToolButton();
    button->setIcon(cIcon);
    button->setCheckable(true);
    bool colorizeFlag = designerSettings().navigatorColorizeIcons();
    button->setChecked(colorizeFlag);
    button->setToolTip(tr("Colorize Component Icons"));
    connect(button, &QAbstractButton::toggled, this, &NavigatorWidget::colorizeToggled);
    buttons.append(button);

    // Show reference nodes
    const QIcon referenceOffIcon = Theme::iconFromName(Theme::Icon::unLinked);
    const QIcon referenceOnIcon = Theme::iconFromName(Theme::Icon::linked);

    QIcon refIcon;
    refIcon.addPixmap(referenceOnIcon.pixmap({16, 16}), QIcon::Normal, QIcon::On);
    refIcon.addPixmap(referenceOffIcon.pixmap({16, 16}), QIcon::Normal, QIcon::Off);

    button = new QToolButton();
    button->setIcon(refIcon);
    button->setCheckable(true);
    bool referenceFlag = designerSettings().navigatorShowReferenceNodes();
    button->setChecked(referenceFlag);
    button->setToolTip(tr("Show reference nodes"));
    connect(button, &QAbstractButton::toggled, this, &NavigatorWidget::referenceToggled);
    buttons.append(button);

    // Show Only Visible Components
    auto visibleIcon = Theme::iconFromName(Theme::Icon::visible_medium);
    auto invisibleIcon = Theme::iconFromName(Theme::Icon::invisible_medium,
                                             Theme::getColor(Theme::Color::DStextSelectedTextColor));
    QIcon vIcon;
    vIcon.addPixmap(invisibleIcon.pixmap({16, 16}), QIcon::Normal, QIcon::On);
    vIcon.addPixmap(visibleIcon.pixmap({16, 16}), QIcon::Normal, QIcon::Off);

    button = new QToolButton();
    button->setIcon(vIcon);
    button->setCheckable(true);
    bool visibleFlag = designerSettings().navigatorShowOnlyVisibleItems();
    button->setChecked(visibleFlag);
    button->setToolTip(tr("Show Only Visible Components"));
    connect(button, &QAbstractButton::toggled, this, &NavigatorWidget::filterToggled);
    buttons.append(button);

    // Reverse Component Order
    auto reverseOffIcon = Theme::iconFromName(Theme::Icon::reverseOrder_medium);
    auto reverseOnIcon = Theme::iconFromName(Theme::Icon::reverseOrder_medium,
                                             Theme::getColor(Theme::Color::DStextSelectedTextColor));
    QIcon rIcon;
    rIcon.addPixmap(reverseOnIcon.pixmap({16, 16}), QIcon::Normal, QIcon::On);
    rIcon.addPixmap(reverseOffIcon.pixmap({16, 16}), QIcon::Normal, QIcon::Off);

    button = new QToolButton();
    button->setIcon(rIcon);
    button->setCheckable(true);
    bool reverseFlag = designerSettings().navigatorReverseItemOrder();
    button->setChecked(reverseFlag);
    button->setToolTip(tr("Reverse Component Order"));
    connect(button, &QAbstractButton::toggled, this, &NavigatorWidget::reverseOrderToggled);
    buttons.append(button);

    empty = new QWidget();
    empty->setFixedWidth(5);
    empty->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    buttons.append(empty);

    return buttons;
}

QToolBar *NavigatorWidget::createToolBar()
{
    NanotraceHR::Tracer tracer{"navigator widget create toolbar", category()};

    const QList<QWidget *> buttons = createToolBarWidgets();

    auto toolBar = new QToolBar();
    toolBar->setFixedHeight(Theme::toolbarSize());
    for (auto toolButton : buttons)
        toolBar->addWidget(toolButton);

    return toolBar;
}

void NavigatorWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    NanotraceHR::Tracer tracer{"navigator widget context help", category()};

    if (auto view = navigatorView()) {
        QmlDesignerPlugin::contextHelp(callback, view->contextHelpId());
    } else {
        callback({});
    }
}

void NavigatorWidget::disableNavigator()
{
    NanotraceHR::Tracer tracer{"navigator widget disable navigator", category()};

    m_treeView->setEnabled(false);
}

void NavigatorWidget::enableNavigator()
{
    NanotraceHR::Tracer tracer{"navigator widget enable navigator", category()};

    m_treeView->setEnabled(true);
}

NavigatorView *NavigatorWidget::navigatorView() const
{
    NanotraceHR::Tracer tracer{"navigator widget navigator view", category()};

    return m_navigatorView.data();
}

void NavigatorWidget::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    NanotraceHR::Tracer tracer{"navigator widget drag enter event", category()};

    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager()
                                                     .designerActionManager();
    if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData()))
        dragEnterEvent->acceptProposedAction();
}

void NavigatorWidget::dropEvent(QDropEvent *dropEvent)
{
    NanotraceHR::Tracer tracer{"navigator widget drop event", category()};

    dropEvent->accept();
    const DesignerActionManager &actionManager = QmlDesignerPlugin::instance()
                                                     ->viewManager().designerActionManager();
    actionManager.handleExternalAssetsDrop(dropEvent->mimeData());
}

void NavigatorWidget::setDragType(const QByteArray &type)
{
    NanotraceHR::Tracer tracer{"navigator widget set drag type", category()};

    m_dragType = type;
}

QByteArray NavigatorWidget::dragType() const
{
    NanotraceHR::Tracer tracer{"navigator widget drag type", category()};

    return m_dragType;
}

void NavigatorWidget::clearSearch()
{
    NanotraceHR::Tracer tracer{"navigator widget clear search", category()};

    m_searchWidget->clear();
}

}
