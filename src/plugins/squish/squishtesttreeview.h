/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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
