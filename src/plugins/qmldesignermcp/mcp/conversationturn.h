// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QString>

namespace QmlDesigner {

struct ConversationTurn {
    enum Type { Message, ToolResults };

    Type type = Message;
    QString role;        // "user", "assistant"
    QJsonArray content;
    QDateTime timestamp;
};

} // namespace QmlDesigner
