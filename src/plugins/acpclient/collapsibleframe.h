// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/theme/theme.h>

#include <QFrame>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils {
class QtcIconDisplay;
class Icon;
} // namespace Utils

namespace AcpClient::Internal {

class CollapsibleFrame : public QFrame
{
    Q_OBJECT

public:
    static constexpr int BodyMarginLeft = 8;
    static constexpr int BodyMarginRight = 8;
    static constexpr int BodyMarginTop = 0;
    static constexpr int BodyMarginBottom = 6;

    explicit CollapsibleFrame(QWidget *parent = nullptr);

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

    void setCollapsible(bool collapsible);
    bool isCollapsible() const { return m_collapsible; }

    void setHeaderVisible(bool visible);

    QHBoxLayout *headerLayout() const { return m_headerLayout; }
    QVBoxLayout *bodyLayout() const { return m_bodyLayout; }

    void setIndicatorColor(Utils::Theme::Color indicatorColor);

signals:
    void collapsedChanged(bool collapsed);

protected:
    // Subclasses add header widgets to m_headerLayout, body widgets to m_bodyLayout
    QHBoxLayout *m_headerLayout = nullptr;
    QVBoxLayout *m_bodyLayout = nullptr;

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Utils::Icon indicatorIcon() const;

    QVBoxLayout *m_outerLayout = nullptr;
    QWidget *m_headerWidget = nullptr;
    QWidget *m_body = nullptr;
    Utils::QtcIconDisplay *m_indicator = nullptr;
    Utils::Theme::Color m_indicatorColor = Utils::Theme::PanelTextColorDark;
    bool m_collapsed = false;
    bool m_collapsible = true;
};

} // namespace AcpClient::Internal
