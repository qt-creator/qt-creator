// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Core { class IEditor; }

namespace FakeVim::Internal {

class FakeVimHandler;

// The FakeVim handler driving the given editor, or nullptr when FakeVim is not
// active there. Defined in fakevimplugin.cpp, which owns the editor-to-handler
// map.
FakeVimHandler *handlerForEditor(Core::IEditor *editor);

void registerMcpTools();

} // namespace FakeVim::Internal
