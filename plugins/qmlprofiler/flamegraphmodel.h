/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        MaxRole
    };

    FlameGraphModel(QmlProfiler::QmlProfilerModelManager *modelManager, QObject *parent = 0);

    void setEventTypeAccepted(QmlDebug::RangeType type, bool accepted);
    bool eventTypeAccepted(QmlDebug::RangeType) const;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
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
