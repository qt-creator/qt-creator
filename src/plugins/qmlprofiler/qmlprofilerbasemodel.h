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

#ifndef QMLPROFILERBASEMODEL_H
#define QMLPROFILERBASEMODEL_H

#include "qmlprofiler_global.h"
#include "qmlprofilerdetailsrewriter.h"
#include <QObject>

namespace QmlProfiler {

class QmlProfilerModelManager;

class QMLPROFILER_EXPORT QmlProfilerBaseModel : public QObject {
    Q_OBJECT
public:
    QmlProfilerBaseModel(Utils::FileInProjectFinder *fileFinder, QmlProfilerModelManager *manager);
    virtual ~QmlProfilerBaseModel() {}

    virtual void complete();
    virtual void clear();
    virtual bool isEmpty() const = 0;
    bool processingDone() const { return m_processingDone; }

    static QString formatTime(qint64 timestamp);

protected:
    QmlProfilerModelManager *m_modelManager;
    int m_modelId;
    bool m_processingDone;
    Internal::QmlProfilerDetailsRewriter m_detailsRewriter;

protected slots:
    virtual void detailsChanged(int requestId, const QString &newString) = 0;
    virtual void detailsDone();

signals:
    void changed();
};

}

#endif // QMLPROFILERBASEMODEL_H
