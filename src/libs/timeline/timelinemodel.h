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

#include "timeline_global.h"
#include "timelinerenderpass.h"
#include <QVariant>
#include <QColor>

namespace Timeline {

class TIMELINE_EXPORT TimelineModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int modelId READ modelId CONSTANT)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY contentChanged)
    Q_PROPERTY(bool hidden READ hidden WRITE setHidden NOTIFY hiddenChanged)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(int expandedRowCount READ expandedRowCount NOTIFY expandedRowCountChanged)
    Q_PROPERTY(int collapsedRowCount READ collapsedRowCount NOTIFY collapsedRowCountChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(QVariantList labels READ labels NOTIFY contentChanged)
    Q_PROPERTY(int count READ count NOTIFY contentChanged)
    Q_PROPERTY(int defaultRowHeight READ defaultRowHeight CONSTANT)

public:
    class TimelineModelPrivate;

    TimelineModel(int modelId, QObject *parent = 0);
    ~TimelineModel();

    // Methods implemented by the abstract model itself
    bool isEmpty() const;
    int modelId() const;

    Q_INVOKABLE int collapsedRowHeight(int rowNumber) const;
    Q_INVOKABLE int expandedRowHeight(int rowNumber) const;
    Q_INVOKABLE int rowHeight(int rowNumber) const;
    Q_INVOKABLE void setExpandedRowHeight(int rowNumber, int height);

    Q_INVOKABLE int collapsedRowOffset(int rowNumber) const;
    Q_INVOKABLE int expandedRowOffset(int rowNumber) const;
    Q_INVOKABLE int rowOffset(int rowNumber) const;

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
    void setDisplayName(const QString &displayName);
    QString displayName() const;
    int expandedRowCount() const;
    int collapsedRowCount() const;
    int rowCount() const;

    // Methods which can optionally be implemented by child models.
    Q_INVOKABLE virtual QRgb color(int index) const;
    virtual QVariantList labels() const;
    Q_INVOKABLE virtual QVariantMap details(int index) const;
    Q_INVOKABLE virtual int expandedRow(int index) const;
    Q_INVOKABLE virtual int collapsedRow(int index) const;
    Q_INVOKABLE int row(int index) const;

    // returned map should contain "file", "line", "column" properties, or be empty
    Q_INVOKABLE virtual QVariantMap location(int index) const;
    Q_INVOKABLE virtual int typeId(int index) const;
    Q_INVOKABLE virtual bool handlesTypeId(int typeId) const;
    Q_INVOKABLE virtual float relativeHeight(int index) const;
    Q_INVOKABLE virtual int rowMinValue(int rowNumber) const;
    Q_INVOKABLE virtual int rowMaxValue(int rowNumber) const;

    Q_INVOKABLE int nextItemBySelectionId(int selectionId, qint64 time, int currentItem) const;
    Q_INVOKABLE int nextItemByTypeId(int typeId, qint64 time, int currentItem) const;
    Q_INVOKABLE int prevItemBySelectionId(int selectionId, qint64 time, int currentItem) const;
    Q_INVOKABLE int prevItemByTypeId(int typeId, qint64 time, int currentItem) const;

    static int defaultRowHeight();
    virtual QList<const TimelineRenderPass *> supportedRenderPasses() const;

signals:
    void expandedChanged();
    void hiddenChanged();
    void expandedRowHeightChanged(int row, int height);
    void contentChanged();
    void heightChanged();
    void expandedRowCountChanged();
    void collapsedRowCountChanged();
    void rowCountChanged();
    void displayNameChanged();

protected:
    QRgb colorBySelectionId(int index) const;
    QRgb colorByFraction(double fraction) const;
    QRgb colorByHue(int hue) const;

    int insert(qint64 startTime, qint64 duration, int selectionId);
    int insertStart(qint64 startTime, int selectionId);
    void insertEnd(int index, qint64 duration);
    void computeNesting();

    void setCollapsedRowCount(int rows);
    void setExpandedRowCount(int rows);

    virtual void clear();

private:
    TimelineModelPrivate *d_ptr;
    Q_DECLARE_PRIVATE(TimelineModel)
};

} // namespace Timeline
