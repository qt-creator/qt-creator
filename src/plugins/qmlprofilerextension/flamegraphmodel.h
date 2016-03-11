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

#ifndef QMLPROFILEREVENTSMODEL_H
#define QMLPROFILEREVENTSMODEL_H

#include <qmlprofiler/qmlprofilerdatamodel.h>
#include <qmlprofiler/qmlprofilernotesmodel.h>
#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qmlprofilereventlocation.h>

#include <QSet>
#include <QVector>
#include <QAbstractItemModel>

namespace QmlProfilerExtension {
namespace Internal {

struct FlameGraphData {
    FlameGraphData(FlameGraphData *parent = 0, int typeIndex = -1, qint64 duration = 0);
    ~FlameGraphData();

    qint64 duration;
    qint64 calls;
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
        TypeId = Qt::UserRole + 1, // Sort by data, not by displayed string
        Type,
        Duration,
        CallCount,
        Details,
        Filename,
        Line,
        Column,
        Note,
        TimePerCall,
        TimeInPercent,
        RangeType,
        Location,
        MaxRole
    };

    FlameGraphModel(QmlProfiler::QmlProfilerModelManager *modelManager, QObject *parent = 0);

    void setEventTypeAccepted(QmlDebug::RangeType type, bool accepted);
    bool eventTypeAccepted(QmlDebug::RangeType) const;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void loadData(qint64 rangeStart = -1, qint64 rangeEnd = -1);
    void loadNotes(int typeId, bool emitSignal);

private:
    friend class FlameGraphRelativesModel;
    friend class FlameGraphParentsModel;
    friend class FlameGraphChildrenModel;

    QVariant lookup(const FlameGraphData &data, int role) const;
    void clear();
    FlameGraphData *pushChild(FlameGraphData *parent,
                              const QmlProfiler::QmlProfilerDataModel::QmlEventData *data);

    int m_selectedTypeIndex;
    FlameGraphData m_stackBottom;

    int m_modelId;
    QmlProfiler::QmlProfilerModelManager *m_modelManager;

    QList<QmlDebug::RangeType> m_acceptedTypes;
    QSet<int> m_typeIdsWithNotes;
};

} // namespace Internal
} // namespace QmlprofilerExtension

#endif // QMLPROFILEREVENTSMODEL_H
