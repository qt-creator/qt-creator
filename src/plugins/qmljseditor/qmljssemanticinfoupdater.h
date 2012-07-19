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

#ifndef SEMANTICINFOUPDATER_H
#define SEMANTICINFOUPDATER_H

#include "qmljseditor.h"

#include <QWaitCondition>
#include <QModelIndex>
#include <QMutex>
#include <QThread>

namespace QmlJSEditor {
namespace Internal {

class SemanticInfoUpdater: public QThread
{
    Q_OBJECT

public:
    SemanticInfoUpdater(QObject *parent = 0);
    virtual ~SemanticInfoUpdater();

    void abort();
    void update(const QmlJS::Document::Ptr &doc, const QmlJS::Snapshot &snapshot);
    void reupdate(const QmlJS::Snapshot &snapshot);

Q_SIGNALS:
    void updated(const QmlJSTools::SemanticInfo &semanticInfo);

protected:
    virtual void run();

private:
    QmlJSTools::SemanticInfo makeNewSemanticInfo(const QmlJS::Document::Ptr &doc,
                                                 const QmlJS::Snapshot &snapshot);

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_wasCancelled;
    QmlJS::Document::Ptr m_sourceDocument;
    QmlJS::Snapshot m_sourceSnapshot;
    QmlJSTools::SemanticInfo m_lastSemanticInfo;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // SEMANTICINFOUPDATER_H
