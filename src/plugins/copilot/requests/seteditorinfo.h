// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "checkstatus.h"

#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/lsptypes.h>

namespace Copilot {

using Key = LanguageServerProtocol::Key;

class EditorPluginInfo : public LanguageServerProtocol::JsonObject
{
    static constexpr Key version{"version"};
    static constexpr Key name{"name"};

public:
    using JsonObject::JsonObject;

    EditorPluginInfo(const QString &version, const QString &name)
    {
        setEditorVersion(version);
        setEditorName(name);
    }

    void setEditorVersion(const QString &v) { insert(version, v); }
    void setEditorName(const QString &n) { insert(name, n); }
};

class EditorInfo : public LanguageServerProtocol::JsonObject
{
    static constexpr Key version{"version"};
    static constexpr Key name{"name"};

public:
    using JsonObject::JsonObject;

    EditorInfo(const QString &version, const QString &name)
    {
        setEditorVersion(version);
        setEditorName(name);
    }

    void setEditorVersion(const QString &v) { insert(version, v); }
    void setEditorName(const QString &n) { insert(name, n); }
};

} // namespace Copilot
