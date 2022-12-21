// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <connectionmanager.h>

#include <QFile>
#include <QPointer>
#include <QProcess>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
QT_END_NAMESPACE

namespace QmlDesigner {

class TestConnectionManager final : public ConnectionManager
{
public:
    TestConnectionManager();

    void writeCommand(const QVariant &command) override;

protected:
    void dispatchCommand(const QVariant &command, Connection &connection) override;

private:
    int m_synchronizeId = -1;
};

} // namespace QmlDesigner
