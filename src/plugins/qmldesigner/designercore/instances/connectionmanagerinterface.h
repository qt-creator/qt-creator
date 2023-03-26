// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qprocessuniqueptr.h"
#include <qmldesignercorelib_global.h>

#include <QTimer>

QT_BEGIN_NAMESPACE
class QLocalSocket;
class QLocalServer;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

class NodeInstanceServerInterface;
class AbstractView;
class ExternalDependenciesInterface;

class QMLDESIGNERCORE_EXPORT ConnectionManagerInterface
{
public:
    class QMLDESIGNERCORE_EXPORT Connection final
    {
    public:
        Connection(const QString &name, const QString &mode);
        Connection(Connection &&connection);

        ~Connection();

        void clear();

    public:
        QString name;
        QString mode;
        QProcessUniquePointer qmlPuppetProcess;
        std::unique_ptr<QLocalSocket> socket;
        std::unique_ptr<QLocalServer> localServer;
        quint32 blockSize = 0;
        quint32 lastReadCommandCounter = 0;
        std::unique_ptr<QTimer> timer;
    };

    virtual ~ConnectionManagerInterface();

    virtual void setUp(NodeInstanceServerInterface *nodeInstanceServer,
                       const QString &qrcMappingString,
                       ProjectExplorer::Target *target,
                       AbstractView *view,
                       ExternalDependenciesInterface &externalDependencies)
        = 0;
    virtual void shutDown() = 0;

    virtual void setCrashCallback(std::function<void()> callback) = 0;

    virtual void writeCommand(const QVariant &command) = 0;

    virtual quint32 writeCounter() const { return 0; }

protected:
    virtual void dispatchCommand(const QVariant &command, Connection &connection) = 0;
    virtual void processFinished(int exitCode, QProcess::ExitStatus exitStatus, const QString &connectionName) = 0;
};

} // namespace QmlDesigner
