/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
