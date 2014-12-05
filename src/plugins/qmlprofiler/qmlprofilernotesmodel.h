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

#ifndef NOTESMODEL_H
#define NOTESMODEL_H

#include "qmlprofilertimelinemodel.h"
#include "qmlprofilermodelmanager.h"
#include <QList>
#include <QHash>

namespace QmlProfiler {
class QMLPROFILER_EXPORT QmlProfilerNotesModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
public:
    struct Note {
        // Saved properties
        QString text;

        // Cache, created on loading
        int timelineModel;
        int timelineIndex;
    };

    QmlProfilerNotesModel(QObject *parent);
    int count() const;

    void setModelManager(QmlProfilerModelManager *modelManager);
    void addTimelineModel(const TimelineModel *timelineModel);

    Q_INVOKABLE int typeId(int index) const;
    Q_INVOKABLE QString text(int index) const;
    Q_INVOKABLE int timelineModel(int index) const;
    Q_INVOKABLE int timelineIndex(int index) const;

    Q_INVOKABLE QVariantList byTypeId(int typeId) const;
    Q_INVOKABLE QVariantList byTimelineModel(int timelineModel) const;

    Q_INVOKABLE int get(int timelineModel, int timelineIndex) const;
    Q_INVOKABLE int add(int timelineModel, int timelineIndex, const QString &text);
    Q_INVOKABLE void update(int index, const QString &text);
    Q_INVOKABLE void remove(int index);

    Q_INVOKABLE void setText(int noteId, const QString &text);
    Q_INVOKABLE void setText(int modelIndex, int index, const QString &text);

    bool isModified() const;
    void loadData();
    void saveData();
    void clear();

signals:
    void changed(int typeId, int timelineModel, int timelineIndex);

private slots:
    void removeTimelineModel(QObject *timelineModel);

protected:
    QmlProfilerModelManager *m_modelManager;
    QList<Note> m_data;
    QHash<int, const TimelineModel *> m_timelineModels;
    bool m_modified;

    int add(int typeId, qint64 startTime, qint64 duration, const QString &text);
};
}
#endif // NOTESMODEL_H
