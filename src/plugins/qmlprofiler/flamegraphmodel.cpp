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

#include "flamegraphmodel.h"

#include "qmlprofilermodelmanager.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QVector>
#include <QString>
#include <QQueue>
#include <QSet>

namespace QmlProfiler {
namespace Internal {

FlameGraphModel::FlameGraphModel(QmlProfilerModelManager *modelManager,
                                 QObject *parent) : QAbstractItemModel(parent)
{
    m_modelManager = modelManager;
    m_callStack.append(QmlEvent());
    m_compileStack.append(QmlEvent());
    m_callStackTop = &m_stackBottom;
    m_compileStackTop = &m_stackBottom;
    connect(modelManager, &QmlProfilerModelManager::stateChanged,
            this, &FlameGraphModel::onModelManagerStateChanged);
    connect(modelManager->notesModel(), &Timeline::TimelineNotesModel::changed,
            this, [this](int typeId, int, int){loadNotes(typeId, true);});
    m_modelId = modelManager->registerModelProxy();

    modelManager->announceFeatures(Constants::QML_JS_RANGE_FEATURES | 1 << ProfileMemory,
                                   [this](const QmlEvent &event, const QmlEventType &type) {
        loadEvent(event, type);
    }, [this](){
        finalize();
    });
}

void FlameGraphModel::clear()
{
    beginResetModel();
    m_stackBottom = FlameGraphData(0, -1, 1);
    m_callStack.clear();
    m_compileStack.clear();
    m_callStack.append(QmlEvent());
    m_compileStack.append(QmlEvent());
    m_callStackTop = &m_stackBottom;
    m_compileStackTop = &m_stackBottom;
    m_typeIdsWithNotes.clear();
    endResetModel();
}

void FlameGraphModel::loadNotes(int typeIndex, bool emitSignal)
{
    QSet<int> changedNotes;
    Timeline::TimelineNotesModel *notes = m_modelManager->notesModel();
    if (typeIndex == -1) {
        changedNotes = m_typeIdsWithNotes;
        m_typeIdsWithNotes.clear();
        for (int i = 0; i < notes->count(); ++i)
            m_typeIdsWithNotes.insert(notes->typeId(i));
        changedNotes += m_typeIdsWithNotes;
    } else {
        changedNotes.insert(typeIndex);
        if (notes->byTypeId(typeIndex).isEmpty())
            m_typeIdsWithNotes.remove(typeIndex);
        else
            m_typeIdsWithNotes.insert(typeIndex);
    }

    if (emitSignal)
        emit dataChanged(QModelIndex(), QModelIndex(), QVector<int>() << NoteRole);
}

void FlameGraphModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    Q_UNUSED(type);

    if (m_stackBottom.children.isEmpty())
        beginResetModel();

    const bool isCompiling = (type.rangeType() == Compiling);
    QStack<QmlEvent> &stack =  isCompiling ? m_compileStack : m_callStack;
    FlameGraphData *&stackTop = isCompiling ? m_compileStackTop : m_callStackTop;
    QTC_ASSERT(stackTop, return);

    if (type.message() == MemoryAllocation) {
        if (type.detailType() == HeapPage)
            return; // We're only interested in actual allocations, not heap pages being mmap'd

        qint64 amount = event.number<qint64>(0);
        if (amount < 0)
            return; // We're not interested in GC runs here

        for (FlameGraphData *data = stackTop; data; data = data->parent) {
            ++data->allocations;
            data->memory += amount;
        }

    } else if (event.rangeStage() == RangeEnd) {
        QTC_ASSERT(stackTop != &m_stackBottom, return);
        QTC_ASSERT(stackTop->typeIndex == event.typeIndex(), return);
        stackTop->duration += event.timestamp() - stack.top().timestamp();
        stack.pop();
        stackTop = stackTop->parent;
    } else {
        QTC_ASSERT(event.rangeStage() == RangeStart, return);
        stack.push(event);
        stackTop = pushChild(stackTop, event);
    }
    QTC_CHECK(stackTop);
}

void FlameGraphModel::finalize()
{
    for (FlameGraphData *child : m_stackBottom.children)
        m_stackBottom.duration += child->duration;

    loadNotes(-1, false);
    endResetModel();
}

void FlameGraphModel::onModelManagerStateChanged()
{
    if (m_modelManager->state() == QmlProfilerModelManager::ClearingData)
        clear();
}

static QString nameForType(RangeType typeNumber)
{
    switch (typeNumber) {
    case Compiling: return FlameGraphModel::tr("Compile");
    case Creating: return FlameGraphModel::tr("Create");
    case Binding: return FlameGraphModel::tr("Binding");
    case HandlingSignal: return FlameGraphModel::tr("Signal");
    case Javascript: return FlameGraphModel::tr("JavaScript");
    default: Q_UNREACHABLE();
    }
}

QVariant FlameGraphModel::lookup(const FlameGraphData &stats, int role) const
{
    switch (role) {
    case TypeIdRole: return stats.typeIndex;
    case NoteRole: {
        QString ret;
        if (!m_typeIdsWithNotes.contains(stats.typeIndex))
            return ret;
        QmlProfilerNotesModel *notes = m_modelManager->notesModel();
        foreach (const QVariant &item, notes->byTypeId(stats.typeIndex)) {
            if (ret.isEmpty())
                ret = notes->text(item.toInt());
            else
                ret += QChar::LineFeed + notes->text(item.toInt());
        }
        return ret;
    }
    case DurationRole: return stats.duration;
    case CallCountRole: return stats.calls;
    case TimePerCallRole: return stats.duration / stats.calls;
    case TimeInPercentRole: return stats.duration * 100 / m_stackBottom.duration;
    case AllocationsRole: return stats.allocations;
    case MemoryRole: return stats.memory;
    default: break;
    }

    if (stats.typeIndex != -1) {
        const QVector<QmlEventType> &typeList = m_modelManager->eventTypes();
        const QmlEventType &type = typeList[stats.typeIndex];

        switch (role) {
        case FilenameRole: return type.location().filename();
        case LineRole: return type.location().line();
        case ColumnRole: return type.location().column();
        case TypeRole: return nameForType(type.rangeType());
        case RangeTypeRole: return type.rangeType();
        case DetailsRole: return type.data().isEmpty() ?
                        FlameGraphModel::tr("Source code not available") : type.data();
        case LocationRole: return type.displayName();
        default: return QVariant();
        }
    } else {
        return QVariant();
    }
}

FlameGraphData::FlameGraphData(FlameGraphData *parent, int typeIndex, qint64 duration) :
    duration(duration), calls(1), memory(0), allocations(0), typeIndex(typeIndex), parent(parent) {}

FlameGraphData::~FlameGraphData()
{
    qDeleteAll(children);
}

FlameGraphData *FlameGraphModel::pushChild(FlameGraphData *parent, const QmlEvent &data)
{
    QVector<FlameGraphData *> &siblings = parent->children;

    for (auto it = siblings.begin(), end = siblings.end(); it != end; ++it) {
        FlameGraphData *child = *it;
        if (child->typeIndex == data.typeIndex()) {
            ++child->calls;
            for (auto back = it, front = siblings.begin(); back != front;) {
                 --back;
                if ((*back)->calls >= (*it)->calls)
                    break;
                qSwap(*it, *back);
                it = back;
            }
            return child;
        }
    }

    FlameGraphData *child = new FlameGraphData(parent, data.typeIndex());
    parent->children.append(child);
    return child;
}

QModelIndex FlameGraphModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        FlameGraphData *parentData = static_cast<FlameGraphData *>(parent.internalPointer());
        return createIndex(row, column, parentData->children[row]);
    } else {
        return createIndex(row, column, row >= 0 ? m_stackBottom.children[row] : 0);
    }
}

QModelIndex FlameGraphModel::parent(const QModelIndex &child) const
{
    if (child.isValid()) {
        FlameGraphData *childData = static_cast<FlameGraphData *>(child.internalPointer());
        return childData->parent == &m_stackBottom ? QModelIndex() :
                                                     createIndex(0, 0, childData->parent);
    } else {
        return QModelIndex();
    }
}

int FlameGraphModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        FlameGraphData *parentData = static_cast<FlameGraphData *>(parent.internalPointer());
        return parentData->children.count();
    } else {
        return m_stackBottom.children.count();
    }
}

int FlameGraphModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant FlameGraphModel::data(const QModelIndex &index, int role) const
{
    FlameGraphData *data = static_cast<FlameGraphData *>(index.internalPointer());
    return lookup(data ? *data : m_stackBottom, role);
}

QHash<int, QByteArray> FlameGraphModel::roleNames() const
{
    static QHash<int, QByteArray> extraRoles{
        {TypeIdRole, "typeId"},
        {TypeRole, "type"},
        {DurationRole, "duration"},
        {CallCountRole, "callCount"},
        {DetailsRole, "details"},
        {FilenameRole, "filename"},
        {LineRole, "line"},
        {ColumnRole, "column"},
        {NoteRole, "note"},
        {TimePerCallRole, "timePerCall"},
        {TimeInPercentRole, "timeInPercent"},
        {RangeTypeRole, "rangeType"},
        {LocationRole, "location" },
        {AllocationsRole, "allocations" },
        {MemoryRole, "memory" }
    };
    return QAbstractItemModel::roleNames().unite(extraRoles);
}

QmlProfilerModelManager *FlameGraphModel::modelManager() const
{
    return m_modelManager;
}

} // namespace Internal
} // namespace QmlProfiler
