// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/navigationtreeview.h>

#include <QModelIndex>
#include <QStyledItemDelegate>

namespace Core { class IContext; }

namespace Squish {
namespace Internal {

class SquishTestTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT
public:
    SquishTestTreeView(QWidget *parent = nullptr);
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void runTestSuite(const QString &suiteName);
    void runTestCase(const QString &suiteName, const QString &testCaseName);
    void openObjectsMap(const QString &suiteName);
    void recordTestCase(const QString &suiteName, const QString &testCaseName);

private:
    Core::IContext *m_context;
    QModelIndex m_lastMousePressedIndex;
};

class SquishTestTreeItemDelegate : public QStyledItemDelegate
{
public:
    SquishTestTreeItemDelegate(QObject *parent = nullptr);
    ~SquishTestTreeItemDelegate() override {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &idx) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

} // namespace Internal
} // namespace Squish
