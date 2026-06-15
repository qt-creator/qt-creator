// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QColor>
#include <QList>
#include <QMap>
#include <QObject>

#include <memory>

namespace Timeline {

class RowLabel
{
public:
    QString description;
    int id = -1;

private:
    friend bool operator==(const RowLabel &, const RowLabel &) = default;
};
using RowLabels = QList<RowLabel>;

class ItemLocation
{
public:
    QString file;
    int line = -1;
    int column = 0;

private:
    friend bool operator==(const ItemLocation &, const ItemLocation &) = default;
};

using ItemDetails = QMap<QString, QString>;

class OrderedItemDetails
{
public:
    QString title;
    QList<std::pair<QString, QString>> content;
};

class TimelineModelAggregator;

class TRACING_EXPORT TimelineModel : public QObject
{
    Q_OBJECT

public:
    class TimelineModelPrivate;

    TimelineModel(TimelineModelAggregator *parent);
    ~TimelineModel() override;

    // Methods implemented by the abstract model itself
    bool isEmpty() const;
    int modelId() const;

    int collapsedRowHeight(int rowNumber) const;
    int expandedRowHeight(int rowNumber) const;
    int rowHeight(int rowNumber) const;
    void setExpandedRowHeight(int rowNumber, int height);

    int collapsedRowOffset(int rowNumber) const;
    int expandedRowOffset(int rowNumber) const;
    int rowOffset(int rowNumber) const;

    int height() const;
    int count() const;
    qint64 duration(int index) const;
    qint64 startTime(int index) const;
    qint64 endTime(int index) const;
    int selectionId(int index) const;

    int firstIndex(qint64 startTime) const;
    int lastIndex(qint64 endTime) const;
    int bestIndex(qint64 timestamp) const;
    int parentIndex(int index) const;

    bool expanded() const;
    bool hidden() const;
    void setExpanded(bool expanded);
    void setHidden(bool hidden);
    void setDisplayName(const QString &displayName);
    QString displayName() const;
    int expandedRowCount() const;
    int collapsedRowCount() const;
    int rowCount() const;

    QString tooltip() const;
    void setTooltip(const QString &text);

    QColor categoryColor() const;
    void setCategoryColor(const QColor &color);

    // if this is disabled, a click on the row label will select the single type it contains
    bool hasMixedTypesInExpandedState() const;
    void setHasMixedTypesInExpandedState(bool value);

    // Methods which can optionally be implemented by child models.
    virtual QRgb color(int index) const;
    virtual RowLabels labels() const;
    virtual ItemDetails details(int index) const;
    virtual OrderedItemDetails orderedDetails(int index) const;
    virtual int expandedRow(int index) const;
    virtual int collapsedRow(int index) const;
    int row(int index) const;

    virtual ItemLocation location(int index) const;
    virtual int typeId(int index) const;
    virtual bool handlesTypeId(int typeId) const;
    virtual float relativeHeight(int index) const;
    virtual qint64 rowMinValue(int rowNumber) const;
    virtual qint64 rowMaxValue(int rowNumber) const;

    // Density-graph models fill each pixel column by the activity in the time
    // it covers, instead of drawing one bar per item. When rendersAsDensity()
    // is true, the track painter calls fillDensityColumns() once per row.
    virtual bool rendersAsDensity() const;
    // Fill `out` (already sized to the column count) with the 0..1 activity
    // fraction of `row` across the equal time columns spanning [startTime,
    // endTime]; return false to fall back to per-item rendering.
    virtual bool fillDensityColumns(int row, qint64 startTime, qint64 endTime,
                                    QList<float> &out) const;

    // Constant colour of a whole row, for density-graph rendering. The default
    // reproduces the per-item lookup (first item in the row); density models
    // that know their row colours cheaply should override.
    virtual QRgb rowColor(int row) const;

    int nextItemBySelectionId(int selectionId, qint64 time, int currentItem) const;
    int nextItemByTypeId(int typeId, qint64 time, int currentItem) const;
    int prevItemBySelectionId(int selectionId, qint64 time, int currentItem) const;
    int prevItemByTypeId(int typeId, qint64 time, int currentItem) const;

    static int defaultRowHeight();

signals:
    void expandedChanged();
    void hiddenChanged();
    void expandedRowHeightChanged(int row, int height);
    void contentChanged();
    void heightChanged();
    void rowCountChanged();
    void displayNameChanged();
    void tooltipChanged();
    void categoryColorChanged();
    void hasMixedTypesInExpandedStateChanged();
    void labelsChanged();
    void detailsChanged();

protected:
    QRgb colorBySelectionId(int index) const;
    QRgb colorByFraction(double fraction) const;
    QRgb colorByHue(int hue) const;

    int insert(qint64 startTime, qint64 duration, int selectionId);
    int insertStart(qint64 startTime, int selectionId);
    void insertEnd(int index, qint64 duration);
    void computeNesting();
    QList<int> computeRows(int *maxlevel) const;

    void setCollapsedRowCount(int rows);
    void setExpandedRowCount(int rows);

    virtual void clear();

private:
    std::unique_ptr<TimelineModelPrivate> d;
};

} // namespace Timeline
