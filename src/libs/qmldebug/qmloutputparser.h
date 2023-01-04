// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebug_global.h"

#include <utils/port.h>
#include <QObject>

namespace QmlDebug {

class QMLDEBUG_EXPORT QmlOutputParser : public QObject
{
    Q_OBJECT
public:
    QmlOutputParser(QObject *parent = nullptr);

    void setNoOutputText(const QString &text);
    void processOutput(const QString &output);

signals:
    void waitingForConnectionOnPort(Utils::Port port);
    void connectionEstablishedMessage();
    void connectingToSocketMessage();
    void errorMessage(const QString &detailedError);
    void unknownMessage(const QString &unknownMessage);
    void noOutputMessage();

private:
    QString m_noOutputText;
    QString m_buffer;
};

} // namespace QmlDebug
