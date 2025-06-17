// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolbox.h"
#include "formeditortracing.h"

#include <theme.h>

#include <utils/stylehelper.h>

#include <QToolBar>
#include <QHBoxLayout>
#include <QDebug>
#include <QFrame>

namespace QmlDesigner {

using FormEditorTracing::category;

ToolBox::ToolBox(QWidget *parentWidget)
    : Utils::StyledBar(parentWidget)
    , m_leftToolBar(new QToolBar(QLatin1String("LeftSidebar"), this))
    , m_rightToolBar(new QToolBar(QLatin1String("RightSidebar"), this))
{
    NanotraceHR::Tracer tracer{"tool box constructor", category()};

    Utils::StyleHelper::setPanelWidget(this, false);
    Utils::StyleHelper::setPanelWidgetSingleRow(this, false);
    setFixedHeight(Theme::toolbarSize());

    m_leftToolBar->setFloatable(true);
    m_leftToolBar->setMovable(true);
    m_leftToolBar->setOrientation(Qt::Horizontal);

    auto horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setSpacing(0);

    Utils::StyleHelper::setPanelWidget(m_leftToolBar, false);
    Utils::StyleHelper::setPanelWidgetSingleRow(m_leftToolBar, false);
    m_leftToolBar->setFixedHeight(Theme::toolbarSize());
    m_leftToolBar->setStyleSheet("QToolBarExtension {margin-top: 5px;}");

    Utils::StyleHelper::setPanelWidget(m_rightToolBar, false);
    Utils::StyleHelper::setPanelWidgetSingleRow(m_rightToolBar, false);
    m_rightToolBar->setFixedHeight(Theme::toolbarSize());
    m_rightToolBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);
    m_rightToolBar->setStyleSheet("QToolBarExtension {margin-top: 5px;}");

    auto stretchToolbar = new QToolBar(this);
    Utils::StyleHelper::setPanelWidget(stretchToolbar, false);
    Utils::StyleHelper::setPanelWidgetSingleRow(stretchToolbar, false);
    stretchToolbar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_rightToolBar->setOrientation(Qt::Horizontal);
    horizontalLayout->addWidget(m_leftToolBar);
    horizontalLayout->addWidget(stretchToolbar);
    horizontalLayout->addWidget(m_rightToolBar);
}

void ToolBox::setLeftSideActions(const QList<QAction*> &actions)
{
    NanotraceHR::Tracer tracer{"tool box set left side actions", category()};

    m_leftToolBar->clear();
    m_leftToolBar->addActions(actions);
    resize(sizeHint());
}

void ToolBox::setRightSideActions(const QList<QAction*> &actions)
{
    NanotraceHR::Tracer tracer{"tool box set right side actions", category()};

    m_rightToolBar->clear();
    m_rightToolBar->addActions(actions);
    resize(sizeHint());
}

void ToolBox::addLeftSideAction(QAction *action)
{
    NanotraceHR::Tracer tracer{"tool box add left side action", category()};

    m_leftToolBar->addAction(action);
}

void ToolBox::addRightSideAction(QAction *action)
{
    NanotraceHR::Tracer tracer{"tool box add right side action", category()};

    m_rightToolBar->addAction(action);
}

QList<QAction*> ToolBox::actions() const
{
    NanotraceHR::Tracer tracer{"tool box actions", category()};

    return m_leftToolBar->actions() + m_rightToolBar->actions();
}

} // namespace QmlDesigner
