// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "collapsibleframe.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace AcpClient::Internal {

static QString expandedIndicator() { return QStringLiteral("▼"); }  // black down-pointing triangle
static QString collapsedIndicator() { return QStringLiteral("▶"); } // black right-pointing triangle

CollapsibleFrame::CollapsibleFrame(QWidget *parent)
    : QFrame(parent)
{
    m_outerLayout = new QVBoxLayout(this);
    m_outerLayout->setContentsMargins(0, 0, 0, 0);
    m_outerLayout->setSpacing(0);

    m_headerWidget = new QWidget(this);
    m_headerWidget->setCursor(Qt::PointingHandCursor);
    m_headerWidget->installEventFilter(this);
    auto *headerRow = new QHBoxLayout(m_headerWidget);
    headerRow->setContentsMargins(8, 6, 8, 6);
    headerRow->setSpacing(4);

    m_indicator = new QLabel(expandedIndicator(), this);
    headerRow->addWidget(m_indicator, 0, Qt::AlignTop);

    m_headerLayout = new QHBoxLayout;
    m_headerLayout->setContentsMargins(0, 0, 0, 0);
    m_headerLayout->setSpacing(4);
    headerRow->addLayout(m_headerLayout, 1);

    m_outerLayout->addWidget(m_headerWidget);

    m_body = new QWidget(this);
    m_bodyLayout = new QVBoxLayout(m_body);
    m_bodyLayout->setContentsMargins(BodyMarginLeft, BodyMarginTop,
                                     BodyMarginRight, BodyMarginBottom);
    m_bodyLayout->setSpacing(2);
    m_outerLayout->addWidget(m_body);
}

void CollapsibleFrame::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed)
        return;
    m_collapsed = collapsed;
    m_body->setVisible(!collapsed);
    m_indicator->setText(collapsed ? collapsedIndicator() : expandedIndicator());
}

void CollapsibleFrame::setCollapsible(bool collapsible)
{
    m_collapsible = collapsible;
    m_indicator->setVisible(collapsible);
    m_headerWidget->setCursor(collapsible ? Qt::PointingHandCursor : Qt::ArrowCursor);
    if (!collapsible)
        setCollapsed(false);
}

void CollapsibleFrame::setHeaderVisible(bool visible)
{
    m_headerWidget->setVisible(visible);
}

bool CollapsibleFrame::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_headerWidget && event->type() == QEvent::MouseButtonRelease
        && m_collapsible) {
        setCollapsed(!m_collapsed);
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

} // namespace AcpClient::Internal
