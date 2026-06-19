// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventlocation.h>
#include <qmldebug/qmleventtype.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include <QAbstractItemModel>
#include <QSet>
#include <QStack>

namespace Profiler::Internal {

class  QmlProfilerModelManager;

struct FlameGraphData
{
    Q_DISABLE_COPY_MOVE(FlameGraphData)

    FlameGraphData(FlameGraphData *parent = nullptr, int typeIndex = -1, qint64 duration = 0);
    ~FlameGraphData();

    void clear()
    {
        duration = 0;
        calls = 1;
        memory = 0;
        allocations = 0;
        typeIndex = -1;
        parent = nullptr;
        qDeleteAll(std::exchange(children, {}));
    }

    qint64 duration = 0;
    qint64 calls = 1;
    qint64 memory = 0;

    int allocations = 0;
    int typeIndex = -1;

    FlameGraphData *parent = nullptr;
    QList<FlameGraphData *> children;
};

class FlameGraphModel final : public QAbstractItemModel
{
    Q_OBJECT

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
        RangeTypeRole,
        LocationRole,
        AllocationsRole,
        MemoryRole,
        MaxRole
    };
    Q_ENUM(Role)

    FlameGraphModel(QmlProfilerModelManager *modelManager, QObject *parent = nullptr);

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const final;
    QModelIndex parent(const QModelIndex &child) const final;
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const final;
    Q_INVOKABLE int columnCount(const QModelIndex &parent = QModelIndex()) const final;
    QVariant data(const QModelIndex &index, int role) const final;
    QHash<int, QByteArray> roleNames() const final;

    QmlProfilerModelManager *modelManager() const;

    void loadEvent(const QmlDebug::QmlEvent &event, const QmlDebug::QmlEventType &type);
    void finalize();
    void onTypeDetailsFinished();
    void restrictToFeatures(quint64 visibleFeatures);
    void loadNotes(int typeId, bool emitSignal);
    void clear();

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);

private:
    QVariant lookup(const FlameGraphData &data, int role) const;
    FlameGraphData *pushChild(FlameGraphData *parent, const QmlDebug::QmlEvent &data);

    // used by binding loop detection
    QStack<QmlDebug::QmlEvent> m_callStack;
    QStack<QmlDebug::QmlEvent> m_compileStack;
    FlameGraphData m_stackBottom;
    FlameGraphData *m_callStackTop;
    FlameGraphData *m_compileStackTop;

    quint64 m_acceptedFeatures;
    QmlProfilerModelManager *m_modelManager;

    QSet<int> m_typeIdsWithNotes;
};

} // namespace Profiler::Internal
