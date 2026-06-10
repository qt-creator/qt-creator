// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include "timelinemodel.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QStandardItemModel;
class QTreeView;
QT_END_NAMESPACE

namespace Timeline {

class TRACING_EXPORT RangeDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RangeDetailsWidget(QWidget *parent = nullptr);

    void setData(const QString &title, const QList<QPair<QString, QString>> &content);
    void clear();

signals:
    void recenterOnItem();

protected:
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void rebuildRows(const QList<QPair<QString, QString>> &content);

    QWidget *m_titleBar = nullptr;
    QLabel *m_titleLabel = nullptr;
    QTreeView *m_treeView = nullptr;
    QStandardItemModel *m_model = nullptr;

    bool m_hasData = false;
};

} // namespace Timeline
