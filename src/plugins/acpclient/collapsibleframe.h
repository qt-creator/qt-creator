// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace AcpClient::Internal {

class CollapsibleFrame : public QFrame
{
public:
    static constexpr int BodyMarginLeft = 8;
    static constexpr int BodyMarginRight = 8;
    static constexpr int BodyMarginTop = 0;
    static constexpr int BodyMarginBottom = 6;

    explicit CollapsibleFrame(QWidget *parent = nullptr)
        : QFrame(parent)
    {
        m_outerLayout = new QVBoxLayout(this);
        m_outerLayout->setContentsMargins(0, 0, 0, 0);
        m_outerLayout->setSpacing(0);

        // Header row: indicator + headerLayout
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

        // Body container
        m_body = new QWidget(this);
        m_bodyLayout = new QVBoxLayout(m_body);
        m_bodyLayout->setContentsMargins(BodyMarginLeft, BodyMarginTop,
                                         BodyMarginRight, BodyMarginBottom);
        m_bodyLayout->setSpacing(2);
        m_outerLayout->addWidget(m_body);
    }

    void setCollapsed(bool collapsed)
    {
        m_collapsed = collapsed;
        m_body->setVisible(!collapsed);
        m_indicator->setText(collapsed ? collapsedIndicator() : expandedIndicator());
    }

    bool isCollapsed() const { return m_collapsed; }

    void setCollapsible(bool collapsible)
    {
        m_collapsible = collapsible;
        m_indicator->setVisible(collapsible);
        m_headerWidget->setCursor(collapsible ? Qt::PointingHandCursor : Qt::ArrowCursor);
        if (!collapsible)
            setCollapsed(false);
    }

    bool isCollapsible() const { return m_collapsible; }

    void setHeaderVisible(bool visible) { m_headerWidget->setVisible(visible); }

    QHBoxLayout *headerLayout() const { return m_headerLayout; }
    QVBoxLayout *bodyLayout() const { return m_bodyLayout; }

protected:
    // Subclasses add header widgets to m_headerLayout, body widgets to m_bodyLayout
    QHBoxLayout *m_headerLayout = nullptr;
    QVBoxLayout *m_bodyLayout = nullptr;

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_headerWidget && event->type() == QEvent::MouseButtonRelease
            && m_collapsible) {
            setCollapsed(!m_collapsed);
            return true;
        }
        return QFrame::eventFilter(watched, event);
    }

private:
    static QString expandedIndicator() { return QStringLiteral("\u25BC"); }  // ▼
    static QString collapsedIndicator() { return QStringLiteral("\u25B6"); } // ▶

    QVBoxLayout *m_outerLayout = nullptr;
    QWidget *m_headerWidget = nullptr;
    QWidget *m_body = nullptr;
    QLabel *m_indicator = nullptr;
    bool m_collapsed = false;
    bool m_collapsible = true;
};

} // namespace AcpClient::Internal
