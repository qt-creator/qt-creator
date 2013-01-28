/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CALLGRINDVISUALISATION_H
#define CALLGRINDVISUALISATION_H

#include <QGraphicsView>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {
class Function;
}
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

#endif // VALGRIND_CALLGRIND_CALLGRINDVISUALISATION_H
