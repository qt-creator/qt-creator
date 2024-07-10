// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sol/sol.hpp"
#include <texteditor/textdocument.h>

namespace Lua {

class LuaTextDocument : public QObject
{
    Q_OBJECT

public:
    LuaTextDocument(TextEditor::TextDocument *document)
        : m_document(document)
    {
        connect(
            m_document,
            &TextEditor::TextDocument::contentsChanged,
            this,
            &LuaTextDocument::contentsChanged);
        return;
    }

    void setChangedCallback(sol::function lua_function) { m_changedCallback = lua_function; }

    void contentsChanged()
    {
        if (m_changedCallback)
            m_changedCallback();
    }

private:
    TextEditor::TextDocument *m_document;
    sol::function m_changedCallback;
};

} // namespace Lua
