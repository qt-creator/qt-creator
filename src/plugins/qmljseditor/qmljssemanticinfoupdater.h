/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SEMANTICINFOUPDATER_H
#define SEMANTICINFOUPDATER_H

#include "qmljseditor.h"

#include <QtCore/QWaitCondition>
#include <QtCore/QModelIndex>
#include <QtCore/QMutex>
#include <QtCore/QThread>

namespace QmlJSEditor {
namespace Internal {

struct SemanticInfoUpdaterSource
{
    QmlJS::Document::Ptr document;
    QmlJS::Snapshot snapshot;

    SemanticInfoUpdaterSource()
    { }

    SemanticInfoUpdaterSource(const QmlJS::Document::Ptr &document,
                              const QmlJS::Snapshot &snapshot)
        : document(document)
        , snapshot(snapshot)
    { }

    bool isValid() const
    { return document; }

    void clear()
    {
        document.clear();
        snapshot = QmlJS::Snapshot();
    }
};

class SemanticInfoUpdater: public QThread
{
    Q_OBJECT

public:
    SemanticInfoUpdater(QObject *parent = 0);
    virtual ~SemanticInfoUpdater();

    void abort();
    void update(const SemanticInfoUpdaterSource &source);

Q_SIGNALS:
    void updated(const QmlJSEditor::SemanticInfo &semanticInfo);

protected:
    virtual void run();

private:
    SemanticInfo makeNewSemanticInfo(const SemanticInfoUpdaterSource &source);

private:
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_wasCancelled;
    SemanticInfoUpdaterSource m_source;
    SemanticInfo m_lastSemanticInfo;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // SEMANTICINFOUPDATER_H
