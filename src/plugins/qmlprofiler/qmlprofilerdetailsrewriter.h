/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLPROFILERDETAILSREWRITER_H
#define QMLPROFILERDETAILSREWRITER_H

#include <QtCore/QObject>

#include "qmljsdebugclient/qmlprofilereventlocation.h"
#include <qmljs/qmljsdocument.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerDetailsRewriter : public QObject
{
    Q_OBJECT
public:
    explicit QmlProfilerDetailsRewriter(QObject *parent);
    ~QmlProfilerDetailsRewriter();

private:
    void rewriteDetailsForLocation(QTextStream &textDoc,QmlJS::Document::Ptr doc, int type,
                                   const QmlJsDebugClient::QmlEventLocation &location);
public slots:
    void requestDetailsForLocation(int type, const QmlJsDebugClient::QmlEventLocation &location);
    void reloadDocuments();
    void documentReady(QmlJS::Document::Ptr doc);
signals:
    void rewriteDetailsString(int type, const QmlJsDebugClient::QmlEventLocation &location,
                              const QString &details);
    void eventDetailsChanged();
private:
    class QmlProfilerDetailsRewriterPrivate;
    QmlProfilerDetailsRewriterPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERDETAILSREWRITER_H
