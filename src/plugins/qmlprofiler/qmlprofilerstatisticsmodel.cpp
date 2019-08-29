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

#include "qmlprofilerstatisticsmodel.h"
#include "qmlprofilermodelmanager.h"

#include <tracing/timelineformattime.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <functional>

namespace QmlProfiler {

QString nameForType(RangeType typeNumber)
{
    switch (typeNumber) {
    case Painting: return QmlProfilerStatisticsModel::tr("Painting");
    case Compiling: return QmlProfilerStatisticsModel::tr("Compiling");
    case Creating: return QmlProfilerStatisticsModel::tr("Creating");
    case Binding: return QmlProfilerStatisticsModel::tr("Binding");
    case HandlingSignal: return QmlProfilerStatisticsModel::tr("Handling Signal");
    case Javascript: return QmlProfilerStatisticsModel::tr("JavaScript");
    default: return QString();
    }
}

double QmlProfilerStatisticsModel::durationPercent(int typeId) const
{
    if (typeId < 0)
        return 0;
    else if (typeId >= m_data.length())
        return 100;
    return double(m_data[typeId].totalNonRecursive()) / double(m_rootDuration) * 100;
}

double QmlProfilerStatisticsModel::durationSelfPercent(int typeId) const
{
    if (typeId < 0 || typeId >= m_data.length())
        return 0;
    return double(m_data[typeId].self) / double(m_rootDuration) * 100;
}

QmlProfilerStatisticsModel::QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager)
    : m_modelManager(modelManager)
{
    connect(modelManager->notesModel(), &Timeline::TimelineNotesModel::changed,
            this, &QmlProfilerStatisticsModel::notesChanged);

    m_acceptedTypes << Compiling << Creating << Binding << HandlingSignal << Javascript;

    modelManager->registerFeatures(Constants::QML_JS_RANGE_FEATURES,
                                   std::bind(&QmlProfilerStatisticsModel::loadEvent, this,
                                             std::placeholders::_1, std::placeholders::_2),
                                   std::bind(&QmlProfilerStatisticsModel::beginResetModel, this),
                                   std::bind(&QmlProfilerStatisticsModel::finalize, this),
                                   std::bind(&QmlProfilerStatisticsModel::clear, this));
}

void QmlProfilerStatisticsModel::restrictToFeatures(quint64 features)
{
    bool didChange = false;
    for (int i = 0; i < MaximumRangeType; ++i) {
        RangeType type = static_cast<RangeType>(i);
        quint64 featureFlag = 1ULL << featureFromRangeType(type);
        if (Constants::QML_JS_RANGE_FEATURES & featureFlag) {
            bool accepted = features & featureFlag;
            if (accepted && !m_acceptedTypes.contains(type)) {
                m_acceptedTypes << type;
                didChange = true;
            } else if (!accepted && m_acceptedTypes.contains(type)) {
                m_acceptedTypes.removeOne(type);
                didChange = true;
            }
        }
    }

    if (!didChange)
        return;

    clear();
    QFutureInterface<void> future;
    auto filter = m_modelManager->rangeFilter(m_modelManager->traceStart(),
                                              m_modelManager->traceEnd());
    m_modelManager->replayQmlEvents(filter(std::bind(&QmlProfilerStatisticsModel::loadEvent, this,
                                                     std::placeholders::_1, std::placeholders::_2)),
                                    std::bind(&QmlProfilerStatisticsModel::beginResetModel, this),
                                    [this]() {
        finalize();
        notesChanged(QmlProfilerStatisticsModel::s_invalidTypeId); // Reload notes
    }, [this](const QString &message) {
        endResetModel();
        if (!message.isEmpty()) {
            emit m_modelManager->error(tr("Could not re-read events from temporary trace file: %1")
                                        .arg(message));
        }
        clear();
    }, future);
}

bool QmlProfilerStatisticsModel::isRestrictedToRange() const
{
    return m_modelManager->isRestrictedToRange();
}

QStringList QmlProfilerStatisticsModel::details(int typeIndex) const
{
    QString data;
    QString displayName;

    if (typeIndex >= 0 && typeIndex < m_modelManager->numEventTypes()) {
        const QmlEventType &type = m_modelManager->eventType(typeIndex);
        displayName = nameForType(type.rangeType());

        const QChar ellipsisChar(0x2026);
        const int maxColumnWidth = 32;

        data = type.data();
        if (data.length() > maxColumnWidth)
            data = data.left(maxColumnWidth - 1) + ellipsisChar;
    }

    return QStringList({
        displayName,
        data,
        QString::number(durationPercent(typeIndex), 'f', 2) + QLatin1Char('%')
    });
}

QString QmlProfilerStatisticsModel::summary(const QVector<int> &typeIds) const
{
    const double cutoff = 0.1;
    const double round = 0.05;
    double maximum = 0;
    double sum = 0;

    for (int typeId : typeIds) {
        const double percentage = durationPercent(typeId);
        if (percentage > maximum)
            maximum = percentage;
        sum += percentage;
    }

    const QLatin1Char percent('%');

    if (sum < cutoff)
        return QLatin1Char('<') + QString::number(cutoff, 'f', 1) + percent;

    if (typeIds.length() == 1)
        return QLatin1Char('~') + QString::number(maximum, 'f', 1) + percent;

    // add/subtract 0.05 to avoid problematic rounding
    if (maximum < cutoff)
        return QChar(0x2264) + QString::number(sum + round, 'f', 1) + percent;

    return QChar(0x2265) + QString::number(qMax(maximum - round, cutoff), 'f', 1) + percent;
}

void QmlProfilerStatisticsModel::clear()
{
    beginResetModel();
    m_rootDuration = 0;
    m_data.clear();
    m_notes.clear();
    m_callStack.clear();
    m_compileStack.clear();
    if (!m_calleesModel.isNull())
        m_calleesModel->clear();
    if (!m_callersModel.isNull())
        m_callersModel->clear();
    endResetModel();
}

void QmlProfilerStatisticsModel::setRelativesModel(QmlProfilerStatisticsRelativesModel *relative,
                                                   QmlProfilerStatisticsRelation relation)
{
    if (relation == QmlProfilerStatisticsCallers)
        m_callersModel = relative;
    else
        m_calleesModel = relative;
}

int QmlProfilerStatisticsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.count() + 1;
}

int QmlProfilerStatisticsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : MaxMainField;
}

QVariant QmlProfilerStatisticsModel::dataForMainEntry(const QModelIndex &index, int role) const
{
    switch (role) {
    case FilterRole:
        return m_rootDuration > 0 ? "+" : "-";
    case TypeIdRole:
        return s_mainEntryTypeId;
    case Qt::ForegroundRole:
        return Utils::creatorTheme()->color(Utils::Theme::Timeline_TextColor);
    case SortRole:
        switch (index.column()) {
        case MainTimeInPercent:
            return 100;
        case MainSelfTimeInPercent:
        case MainSelfTime:
            return 0;
        case MainTotalTime:
        case MainTimePerCall:
        case MainMedianTime:
        case MainMaxTime:
        case MainMinTime:
            return m_rootDuration;
        }
        Q_FALLTHROUGH();
    case Qt::DisplayRole:
        switch (index.column()) {
        case MainLocation:
            return "<program>";
        case MainTimeInPercent:
            return "100 %";
        case MainSelfTimeInPercent:
            return "0.00 %";
        case MainSelfTime:
            return Timeline::formatTime(0);
        case MainCallCount:
            return m_rootDuration > 0 ? 1 : 0;
        case MainTotalTime:
        case MainTimePerCall:
        case MainMedianTime:
        case MainMaxTime:
        case MainMinTime:
            return Timeline::formatTime(m_rootDuration);
        case MainDetails:
            return tr("Main program");
        default:
            break;
        }
        Q_FALLTHROUGH();
    default:
        return QVariant();
    }
}

QVariant QmlProfilerStatisticsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() == m_data.count())
        return dataForMainEntry(index, role);

    const int typeIndex = index.row();
    const QmlEventType &type = m_modelManager->eventType(typeIndex);
    const QmlEventStats &stats = m_data.at(typeIndex);

    switch (role) {
    case FilterRole:
        return stats.calls > 0 ? "+" : "-";
    case TypeIdRole:
        return typeIndex;
    case FilenameRole:
        return type.location().filename();
    case LineRole:
        return type.location().line();
    case ColumnRole:
        return type.location().column();
    case Qt::ToolTipRole:
        if (stats.recursive > 0) {
            return (tr("+%1 in recursive calls")
                    .arg(Timeline::formatTime(stats.recursive)));
        } else {
            auto it = m_notes.constFind(typeIndex);
            return it == m_notes.constEnd() ? QString() : it.value();
        }
    case Qt::ForegroundRole:
        return (stats.recursive > 0 || m_notes.contains(typeIndex))
                ? Utils::creatorTheme()->color(Utils::Theme::Timeline_HighlightColor)
                : Utils::creatorTheme()->color(Utils::Theme::Timeline_TextColor);
    case SortRole:
        switch (index.column()) {
        case MainLocation:
            return type.displayName();
        case MainTimeInPercent:
            return durationPercent(typeIndex);
        case MainTotalTime:
            return stats.totalNonRecursive();
        case MainSelfTimeInPercent:
            return durationSelfPercent(typeIndex);
        case MainSelfTime:
            return stats.self;
        case MainTimePerCall:
            return stats.average();
        case MainMedianTime:
            return stats.median;
        case MainMaxTime:
            return stats.maximum;
        case MainMinTime:
            return stats.minimum;
        case MainDetails:
            return type.data();
        default:
            break;
        }
        Q_FALLTHROUGH(); // Rest is same as Qt::DisplayRole
    case Qt::DisplayRole:
        switch (index.column()) {
        case MainLocation:
            return type.displayName().isEmpty() ? tr("<bytecode>") : type.displayName();
        case MainType:
            return nameForType(type.rangeType());
        case MainTimeInPercent:
            return QString::fromLatin1("%1 %").arg(durationPercent(typeIndex), 0, 'f', 2);
        case MainTotalTime:
            return Timeline::formatTime(stats.totalNonRecursive());
        case MainSelfTimeInPercent:
            return QString::fromLatin1("%1 %").arg(durationSelfPercent(typeIndex), 0, 'f', 2);
        case MainSelfTime:
            return Timeline::formatTime(stats.self);
        case MainCallCount:
            return stats.calls;
        case MainTimePerCall:
            return Timeline::formatTime(stats.average());
        case MainMedianTime:
            return Timeline::formatTime(stats.median);
        case MainMaxTime:
            return Timeline::formatTime(stats.maximum);
        case MainMinTime:
            return Timeline::formatTime(stats.minimum);
        case MainDetails:
            return type.data().isEmpty() ? tr("Source code not available")
                                         : type.data();
        default:
            QTC_ASSERT(false, return QVariant());
        }
    default:
        return QVariant();
    }
}

QVariant QmlProfilerStatisticsModel::headerData(int section, Qt::Orientation orientation,
                                                int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case MainCallCount: return tr("Calls");
    case MainDetails: return tr("Details");
    case MainLocation: return tr("Location");
    case MainMaxTime: return tr("Longest Time");
    case MainTimePerCall: return tr("Mean Time");
    case MainSelfTime: return tr("Self Time");
    case MainSelfTimeInPercent: return tr("Self Time in Percent");
    case MainMinTime: return tr("Shortest Time");
    case MainTimeInPercent: return tr("Time in Percent");
    case MainTotalTime: return tr("Total Time");
    case MainType: return tr("Type");
    case MainMedianTime: return tr("Median Time");
    case MaxMainField:
    default: QTC_ASSERT(false, return QString());
    }
}

void QmlProfilerStatisticsModel::typeDetailsChanged(int typeIndex)
{
    const QModelIndex index = createIndex(typeIndex, MainDetails);
    emit dataChanged(index, index, QVector<int>({SortRole, Qt::DisplayRole}));
}

void QmlProfilerStatisticsModel::notesChanged(int typeIndex)
{
    static const QVector<int> noteRoles({Qt::ToolTipRole, Qt::ForegroundRole});
    const Timeline::TimelineNotesModel *notesModel = m_modelManager->notesModel();
    if (typeIndex == s_invalidTypeId) {
        m_notes.clear();
        for (int noteId = 0; noteId < notesModel->count(); ++noteId) {
            int noteType = notesModel->typeId(noteId);
            if (noteType != s_invalidTypeId) {
                QString &note = m_notes[noteType];
                if (note.isEmpty()) {
                    note = notesModel->text(noteId);
                } else {
                    note.append(QStringLiteral("\n")).append(notesModel->text(noteId));
                }
                emit dataChanged(index(noteType, 0), index(noteType, MainDetails), noteRoles);
            }
        }
    } else {
        m_notes.remove(typeIndex);
        const QVariantList changedNotes = notesModel->byTypeId(typeIndex);
        if (!changedNotes.isEmpty()) {
            QStringList newNotes;
            for (QVariantList::ConstIterator it = changedNotes.begin(); it !=  changedNotes.end();
                 ++it) {
                newNotes << notesModel->text(it->toInt());
            }
            m_notes[typeIndex] = newNotes.join(QStringLiteral("\n"));
            emit dataChanged(index(typeIndex, 0), index(typeIndex, MainDetails), noteRoles);
        }
    }
}

void QmlProfilerStatisticsModel::loadEvent(const QmlEvent &event, const QmlEventType &type)
{
    if (!m_acceptedTypes.contains(type.rangeType()))
        return;

    const int typeIndex = event.typeIndex();
    bool isRecursive = false;
    QStack<QmlEvent> &stack = type.rangeType() == Compiling ? m_compileStack : m_callStack;
    switch (event.rangeStage()) {
    case RangeStart:
        stack.push(event);
        if (m_data.length() <= typeIndex)
            m_data.resize(m_modelManager->numEventTypes());
        break;
    case RangeEnd: {
        // update stats
        QTC_ASSERT(!stack.isEmpty(), return);
        QTC_ASSERT(stack.top().typeIndex() == typeIndex, return);
        QmlEventStats &stats = m_data[typeIndex];
        qint64 duration = event.timestamp() - stack.top().timestamp();
        stats.total += duration;
        stats.self += duration;
        stats.durations.push_back(duration);
        stack.pop();

        // recursion detection: check whether event was already in stack
        for (int ii = 0; ii < stack.size(); ++ii) {
            if (stack.at(ii).typeIndex() == typeIndex) {
                isRecursive = true;
                stats.recursive += duration;
                break;
            }
        }

        if (!stack.isEmpty())
            m_data[stack.top().typeIndex()].self -= duration;
        else
            m_rootDuration += duration;

        break;
    }
    default:
        return;
    }

    if (!m_calleesModel.isNull())
        m_calleesModel->loadEvent(type.rangeType(), event, isRecursive);
    if (!m_callersModel.isNull())
        m_callersModel->loadEvent(type.rangeType(), event, isRecursive);
}

void QmlProfilerStatisticsModel::finalize()
{
    for (QmlEventStats &stats : m_data)
        stats.finalize();
    endResetModel();
}

QmlProfilerStatisticsRelativesModel::QmlProfilerStatisticsRelativesModel(
        QmlProfilerModelManager *modelManager,
        QmlProfilerStatisticsModel *statisticsModel,
        QmlProfilerStatisticsRelation relation) :
    m_modelManager(modelManager), m_relation(relation)
{
    QTC_CHECK(modelManager);
    QTC_CHECK(statisticsModel);
    statisticsModel->setRelativesModel(this, relation);
    connect(m_modelManager, &QmlProfilerModelManager::typeDetailsChanged,
            this, &QmlProfilerStatisticsRelativesModel::typeDetailsChanged);
}

bool operator<(const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData &a,
               const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData &b)
{
    return a.typeIndex < b.typeIndex;
}

bool operator<(const QmlProfilerStatisticsRelativesModel::QmlStatisticsRelativesData &a,
               int typeIndex)
{
    return a.typeIndex < typeIndex;
}

void QmlProfilerStatisticsRelativesModel::loadEvent(RangeType type, const QmlEvent &event,
                                                    bool isRecursive)
{
    QStack<Frame> &stack = (type == Compiling) ? m_compileStack : m_callStack;

    switch (event.rangeStage()) {
    case RangeStart:
        stack.push(Frame(event.timestamp(), event.typeIndex()));
        break;
    case RangeEnd: {
        int callerTypeIndex = stack.count() > 1 ? stack[stack.count() - 2].typeId
                                                : QmlProfilerStatisticsModel::s_mainEntryTypeId;
        int relativeTypeIndex = (m_relation == QmlProfilerStatisticsCallers) ? callerTypeIndex :
                                                                               event.typeIndex();
        int selfTypeIndex = (m_relation == QmlProfilerStatisticsCallers) ? event.typeIndex() :
                                                                           callerTypeIndex;

        QVector<QmlStatisticsRelativesData> &relatives = m_data[selfTypeIndex];
        auto it = std::lower_bound(relatives.begin(), relatives.end(), relativeTypeIndex);
        if (it != relatives.end() && it->typeIndex == relativeTypeIndex) {
            it->calls++;
            it->duration += event.timestamp() - stack.top().startTime;
            it->isRecursive = isRecursive || it->isRecursive;
        } else {
            relatives.insert(it, QmlStatisticsRelativesData(
                                 event.timestamp() - stack.top().startTime, 1, relativeTypeIndex,
                                 isRecursive));
        }
        stack.pop();
        break;
    }
    default:
        break;
    }
}

int QmlProfilerStatisticsRelativesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        auto it = m_data.constFind(m_relativeTypeIndex);
        return it == m_data.constEnd() ? 0 : it->count();
    }
}

int QmlProfilerStatisticsRelativesModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : MaxRelativeField;
}

QVariant QmlProfilerStatisticsRelativesModel::dataForMainEntry(qint64 totalDuration, int role,
                                                               int column) const
{
    switch (role) {
    case TypeIdRole:
        return QmlProfilerStatisticsModel::s_mainEntryTypeId;
    case Qt::ForegroundRole:
        return Utils::creatorTheme()->color(Utils::Theme::Timeline_TextColor);
    case SortRole:
        if (column == RelativeTotalTime)
            return totalDuration;
        Q_FALLTHROUGH(); // rest is same as Qt::DisplayRole
    case Qt::DisplayRole:
        switch (column) {
        case RelativeLocation: return "<program>";
        case RelativeTotalTime: return Timeline::formatTime(totalDuration);
        case RelativeCallCount: return 1;
        case RelativeDetails: return tr("Main Program");
        }
    }
    return QVariant();
}

QVariant QmlProfilerStatisticsRelativesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const int row = index.row();

    auto main_it = m_data.find(m_relativeTypeIndex);
    QTC_ASSERT(main_it != m_data.end(), return QVariant());

    const QVector<QmlStatisticsRelativesData> &data = main_it.value();
    QTC_ASSERT(row >= 0 && row < data.length(), return QVariant());

    const QmlStatisticsRelativesData &stats = data.at(row);
    QTC_ASSERT(stats.typeIndex >= 0, return QVariant());

    if (stats.typeIndex == QmlProfilerStatisticsModel::s_mainEntryTypeId)
        return dataForMainEntry(stats.duration, role, index.column());

    QTC_ASSERT(stats.typeIndex < m_modelManager->numEventTypes(), return QVariant());
    const QmlEventType &type = m_modelManager->eventType(stats.typeIndex);

    switch (role) {
    case TypeIdRole:
        return stats.typeIndex;
    case FilenameRole:
        return type.location().filename();
    case LineRole:
        return type.location().line();
    case ColumnRole:
        return type.location().column();
    case Qt::ToolTipRole:
        return stats.isRecursive ? tr("called recursively") : QString();
    case Qt::ForegroundRole:
        return stats.isRecursive
                ? Utils::creatorTheme()->color(Utils::Theme::Timeline_HighlightColor)
                : Utils::creatorTheme()->color(Utils::Theme::Timeline_TextColor);
    case SortRole:
        switch (index.column()) {
        case RelativeLocation:
            return type.displayName();
        case RelativeTotalTime:
            return stats.duration;
        case RelativeDetails:
            return type.data();
        default: break;
        }
        Q_FALLTHROUGH(); // rest is same as Qt::DisplayRole
    case Qt::DisplayRole:
        switch (index.column()) {
        case RelativeLocation:
            return type.displayName().isEmpty() ? tr("<bytecode>") : type.displayName();
        case RelativeType:
            return nameForType(type.rangeType());
        case RelativeTotalTime:
            return Timeline::formatTime(stats.duration);
        case RelativeCallCount:
            return stats.calls;
        case RelativeDetails:
            return type.data().isEmpty() ? tr("Source code not available")
                                         : type.data();
        default:
            QTC_ASSERT(false, return QVariant());
        }
    default:
        return QVariant();
    }
}

QVariant QmlProfilerStatisticsRelativesModel::headerData(int section, Qt::Orientation orientation,
                                                         int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case RelativeLocation:
        return m_relation == QmlProfilerStatisticsCallees ? tr("Callee") : tr("Caller");
    case RelativeType:
        return tr("Type");
    case RelativeTotalTime:
        return tr("Total Time");
    case RelativeCallCount:
        return tr("Calls");
    case RelativeDetails:
        return m_relation == QmlProfilerStatisticsCallees ? tr("Callee Description")
                                                          : tr("Caller Description");
    case MaxRelativeField:
    default:
        QTC_ASSERT(false, return QString());
    }
}

bool QmlProfilerStatisticsRelativesModel::setData(const QModelIndex &index, const QVariant &value,
                                                  int role)
{
    bool ok = false;
    const int typeIndex = value.toInt(&ok);
    if (index.isValid() || !ok || role != TypeIdRole) {
        return QAbstractTableModel::setData(index, value, role);
    } else {
        beginResetModel();
        m_relativeTypeIndex = typeIndex;
        endResetModel();
        return true;
    }
}

void QmlProfilerStatisticsRelativesModel::typeDetailsChanged(int typeId)
{
    auto main_it = m_data.constFind(m_relativeTypeIndex);
    if (main_it == m_data.constEnd())
        return;

    const QVector<QmlStatisticsRelativesData> &rows = main_it.value();
    for (int row = 0, end = rows.length(); row != end; ++row) {
        if (rows[row].typeIndex == typeId) {
            const QModelIndex index = createIndex(row, RelativeDetails);
            emit dataChanged(index, index, QVector<int>({SortRole, Qt::DisplayRole}));
            return;
        }
    }
}

void QmlProfilerStatisticsRelativesModel::clear()
{
    beginResetModel();
    m_relativeTypeIndex = QmlProfilerStatisticsModel::s_invalidTypeId;
    m_data.clear();
    m_callStack.clear();
    m_compileStack.clear();
    endResetModel();
}

} // namespace QmlProfiler
