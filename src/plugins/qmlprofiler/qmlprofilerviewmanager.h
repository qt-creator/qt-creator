/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
