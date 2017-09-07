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

#include "qmlprofilernotesmodel.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"

#include <QSet>
#include <QVector>
#include <QStack>
#include <QAbstractItemModel>

namespace QmlProfiler {
namespace Internal {

struct FlameGraphData {
    FlameGraphData(FlameGraphData *parent = 0, int typeIndex = -1, qint64 duration = 0);
    ~FlameGraphData();

    qint64 duration;
    qint64 calls;
    qint64 memory;

    int allocations;
    int typeIndex;

    FlameGraphData *parent;
    QVector<FlameGraphData *> children;
};

class FlameGraphModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_ENUMS(Role)
public:
    enum Role {
        TypeIdRole = Qt::UserRole + 1, // Sort by data, not by displayed string
        TypeRole,
        DurationRole,
        CallCountRole,
        DetailsRole,
        FilenameRole,
        LineRole,
        ColumnRole,
        NoteRole,
        TimePerCallRole,
        TimeInPercentRole,
        RangeTypeRole,
        LocationRole,
        AllocationsRole,
        MemoryRole,
        MaxRole
    };

    FlameGraphModel(QmlProfilerModelManager *modelManager, QObject *parent = 0);

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QmlProfilerModelManager *modelManager() const;

    void loadEvent(const QmlEvent &event, const QmlEventType &type);
    void finalize();
    void onModelManagerStateChanged();
    void restrictToFeatures(quint64 visibleFeatures);
    void loadNotes(int typeId, bool emitSignal);
    void clear();

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeSelected(int typeIndex);

private:
    QVariant lookup(const FlameGraphData &data, int role) const;
    FlameGraphData *pushChild(FlameGraphData *parent, const QmlEvent &data);

    // used by binding loop detection
    QStack<QmlEvent> m_callStack;
    QStack<QmlEvent> m_compileStack;
    FlameGraphData m_stackBottom;
    FlameGraphData *m_callStackTop;
    FlameGraphData *m_compileStackTop;

    quint64 m_acceptedFeatures;
    int m_modelId;
    QmlProfilerModelManager *m_modelManager;

    QSet<int> m_typeIdsWithNotes;
};

} // namespace Internal
} // namespace QmlProfiler
