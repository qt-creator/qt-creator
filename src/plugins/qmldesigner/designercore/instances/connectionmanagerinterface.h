/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
                       AbstractView *view)
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
