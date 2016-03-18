/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
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

#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind { class Function; }
}

namespace Valgrind {
namespace Internal {

class Visualisation : public QGraphicsView
{
    Q_OBJECT

public:
    explicit Visualisation(QWidget *parent = 0);
    virtual ~Visualisation();

    void setModel(QAbstractItemModel *model);

    const Valgrind::Callgrind::Function *functionForItem(QGraphicsItem *item) const;
    QGraphicsItem *itemForFunction(const Valgrind::Callgrind::Function *function) const;

    void setFunction(const Valgrind::Callgrind::Function *function);
    const Valgrind::Callgrind::Function *function() const;

    void setMinimumInclusiveCostRatio(double ratio);

public slots:
    void setText(const QString &message);

signals:
    void functionActivated(const Valgrind::Callgrind::Function *);
    void functionSelected(const Valgrind::Callgrind::Function *);

protected slots:
    void populateScene();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    class Private;
    Private *d;
};

} // namespace Internal
} // namespace Valgrind
