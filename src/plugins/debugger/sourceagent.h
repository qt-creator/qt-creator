// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Debugger::Internal {

class DebuggerEngine;

class SourceAgent
{
public:
    explicit SourceAgent(DebuggerEngine *engine);
    ~SourceAgent();
    void setSourceProducerName(const QString &name);
    void resetLocation();
    void setContent(const QString &name, const QString &content);
    void updateLocationMarker();

private:
    class SourceAgentPrivate *d;
};

} // Debugger::Internal
