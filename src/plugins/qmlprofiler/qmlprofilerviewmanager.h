/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QMLPROFILERVIEWMANAGER_H
#define QMLPROFILERVIEWMANAGER_H

#include <QObject>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool;
class QmlProfilerDataModel;
class QmlProfilerStateManager;

class QmlProfilerViewManager : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerViewManager(QObject *parent,
                                    QmlProfilerTool *profilerTool,
                                    QmlProfilerDataModel *model,
                                    QmlProfilerStateManager *profilerState);
    ~QmlProfilerViewManager();

    void createViews();

    // used by the options "limit events to range"
    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;
    bool hasGlobalStats() const;
    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);

public slots:
    void clear();

signals:
    void gotoSourceLocation(QString,int,int);

private:
    class QmlProfilerViewManagerPrivate;
    QmlProfilerViewManagerPrivate *d;
};


} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERVIEWMANAGER_H
