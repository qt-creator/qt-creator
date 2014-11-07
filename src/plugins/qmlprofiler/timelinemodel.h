/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TIMELINEMODEL_H
#define TIMELINEMODEL_H


#include "qmlprofiler_global.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilerdatamodel.h"
#include <QVariant>
#include <QColor>

namespace QmlProfiler {

class QMLPROFILER_EXPORT TimelineModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int modelId READ modelId CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY emptyChanged)
    Q_PROPERTY(bool hidden READ hidden WRITE setHidden NOTIFY hiddenChanged)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(QVariantList labels READ labels NOTIFY labelsChanged)
    Q_PROPERTY(int count READ count NOTIFY emptyChanged)
    Q_PROPERTY(int defaultRowHeight READ defaultRowHeight CONSTANT)

public:
    class TimelineModelPrivate;

    TimelineModel(int modelId, const QString &displayName, QObject *parent);
    ~TimelineModel();

    // Methods implemented by the abstract model itself
    bool isEmpty() const;
    int modelId() const;
    Q_INVOKABLE int rowHeight(int rowNumber) const;
    Q_INVOKABLE int rowOffset(int rowNumber) const;
    Q_INVOKABLE void setRowHeight(int rowNumber, int height);
    int height() const;
    int count() const;
    Q_INVOKABLE qint64 duration(int index) const;
    Q_INVOKABLE qint64 startTime(int index) const;
    Q_INVOKABLE qint64 endTime(int index) const;
    Q_INVOKABLE int selectionId(int index) const;

    int firstIndex(qint64 startTime) const;
    int lastIndex(qint64 endTime) const;

    bool expanded() const;
    bool hidden() const;
    void setExpanded(bool expanded);
    void setHidden(bool hidden);
    QString displayName() const;
    int rowCount() const;

    // Methods that have to be implemented by child models
    Q_INVOKABLE virtual QColor color(int index) const = 0;
    virtual QVariantList labels() const = 0;
    Q_INVOKABLE virtual QVariantMap details(int index) const = 0;
    Q_INVOKABLE virtual int row(int index) const = 0;

    // Methods which can optionally be implemented by child models.
    // returned map should contain "file", "line", "column" properties, or be empty
    Q_INVOKABLE virtual QVariantMap location(int index) const;
    Q_INVOKABLE virtual int typeId(int index) const;
    Q_INVOKABLE virtual bool handlesTypeId(int typeId) const;
    Q_INVOKABLE virtual int selectionIdForLocation(const QString &filename, int line,
                                                   int column) const;
    Q_INVOKABLE virtual float relativeHeight(int index) const;
    Q_INVOKABLE virtual int rowMinValue(int rowNumber) const;
    Q_INVOKABLE virtual int rowMaxValue(int rowNumber) const;

    Q_INVOKABLE int nextItemBySelectionId(int selectionId, qint64 time, int currentItem) const;
    Q_INVOKABLE int nextItemByTypeId(int typeId, qint64 time, int currentItem) const;
    Q_INVOKABLE int prevItemBySelectionId(int selectionId, qint64 time, int currentItem) const;
    Q_INVOKABLE int prevItemByTypeId(int typeId, qint64 time, int currentItem) const;

    static int defaultRowHeight();

signals:
    void expandedChanged();
    void hiddenChanged();
    void rowHeightChanged(int row, int height);
    void emptyChanged();
    void heightChanged();
    void rowCountChanged();
    void labelsChanged();

protected:
    QColor colorBySelectionId(int index) const;
    QColor colorByFraction(double fraction) const;
    QColor colorByHue(int hue) const;

    int insert(qint64 startTime, qint64 duration, int selectionId);
    int insertStart(qint64 startTime, int selectionId);
    void insertEnd(int index, qint64 duration);
    void computeNesting();

    int collapsedRowCount() const;
    void setCollapsedRowCount(int rows);

    int expandedRowCount() const;
    void setExpandedRowCount(int rows);

    virtual void clear();

    explicit TimelineModel(TimelineModelPrivate &dd, QObject *parent);
    TimelineModelPrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(TimelineModel)
};

}

#endif // TIMELINEMODEL_H
