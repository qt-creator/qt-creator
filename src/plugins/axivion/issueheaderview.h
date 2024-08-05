// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHeaderView>
#include <QList>

#include <optional>

namespace Axivion::Internal {

class IssueHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    struct ColumnInfo
    {
        int width = 0;
        std::optional<Qt::SortOrder> sortOrder = std::nullopt;
        bool sortable = false;
    };

    explicit IssueHeaderView(QWidget *parent = nullptr) : QHeaderView(Qt::Horizontal, parent) {}
    void setColumnInfoList(const QList<ColumnInfo> &infos);

    std::optional<Qt::SortOrder> currentSortOrder() const { return m_currentSortOrder; }
    int currentSortColumn() const;

signals:
    void sortTriggered();

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
    QSize sectionSizeFromContents(int logicalIndex) const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void onToggleSort(int index, Qt::SortOrder order);
    bool m_dragging = false;
    bool m_maybeToggleSort = false;
    int m_lastToggleLogicalPos = -1;
    int m_currentSortIndex = -1;
    std::optional<Qt::SortOrder> m_currentSortOrder = std::nullopt;
    QList<ColumnInfo> m_columnInfoList;
};

} // namespace Axivion::Internal
