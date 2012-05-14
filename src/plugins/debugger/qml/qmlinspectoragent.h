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

#ifndef QMLINSPECTORAGENT_H
#define QMLINSPECTORAGENT_H

#include <QObject>
#include <QStack>
#include <QTimer>

#include <qmldebug/baseenginedebugclient.h>
#include <watchdata.h>

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class WatchData;

//map <filename, editorRevision> -> <lineNumber, columnNumber> -> debugId
typedef
QHash<QPair<QString, int>, QHash<QPair<int, int>, QList<int> > > DebugIdHash;

class QmlInspectorAgent : public QObject
{
    Q_OBJECT
public:
    explicit QmlInspectorAgent(DebuggerEngine *engine, QObject *parent = 0);


    void fetchObject(int debugId);
    quint32 queryExpressionResult(int debugId, const QString &expression);

    void updateWatchData(const WatchData &data);
    void selectObjectInTree(int debugId);

    quint32 setBindingForObject(int objectDebugId,
                                const QString &propertyName,
                                const QVariant &value,
                                bool isLiteralValue,
                                QString source,
                                int line);
    quint32 setMethodBodyForObject(int objectDebugId, const QString &methodName,
                                   const QString &methodBody);
    quint32 resetBindingForObject(int objectDebugId,
                                  const QString &propertyName);

    QmlDebug::ObjectReference objectForId(const QString &objectId) const;
    int objectIdForLocation(int line, int column) const;
    QHash<int, QString> rootObjectIds() const;
    DebugIdHash debugIdHash() const { return m_debugIdHash; }

    bool addObjectWatch(int objectDebugId);
    bool isObjectBeingWatched(int objectDebugId);
    bool removeObjectWatch(int objectDebugId);
    void removeAllObjectWatches();

    void setEngineClient(QmlDebug::BaseEngineDebugClient *client);
    QString displayName(int objectDebugId) const;

public slots:
    void fetchContextObjectsForLocation(const QString &file,
                                         int lineNumber, int columnNumber);
    void queryEngineContext();

signals:
    void objectTreeUpdated();
    void objectFetched(const QmlDebug::ObjectReference &ref);
    void expressionResult(quint32 queryId, const QVariant &value);
    void propertyChanged(int debugId, const QByteArray &propertyName,
                         const QVariant &propertyValue);

private slots:
    void updateStatus();
    void onResult(quint32 queryId, const QVariant &value, const QByteArray &type);
    void newObject(int engineId, int objectId, int parentId);

private:
    void reloadEngines();
    void fetchObjectsInContextRecursive(const QmlDebug::ContextReference &context);

    void objectTreeFetched(const QmlDebug::ObjectReference &result);

    void buildDebugIdHashRecursive(const QmlDebug::ObjectReference &ref);
    QList<WatchData> buildWatchData(const QmlDebug::ObjectReference &obj,
                                           const QByteArray &parentIname);

    enum LogDirection {
        LogSend,
        LogReceive
    };
    void log(LogDirection direction, const QString &message);

    bool isConnected() const;
    void clearObjectTree();

private:
    DebuggerEngine *m_debuggerEngine;
    QmlDebug::BaseEngineDebugClient *m_engineClient;

    quint32 m_engineQueryId;
    quint32 m_rootContextQueryId;
    int m_objectToSelect;
    QList<quint32> m_objectTreeQueryIds;
    QStack<QmlDebug::ObjectReference> m_objectStack;
    QmlDebug::EngineReference m_engine;
    QHash<int, QByteArray> m_debugIdToIname;
    QHash<int, QList<int> > m_debugIdChildIds; // This is for 4.x
    QHash<int, QmlDebug::FileReference> m_debugIdLocations;
    DebugIdHash m_debugIdHash;

    QList<int> m_objectWatches;
    QList<int> m_fetchDataIds;
    QTimer m_delayQueryTimer;
};

} // Internal
} // Debugger

#endif // QMLINSPECTORAGENT_H
