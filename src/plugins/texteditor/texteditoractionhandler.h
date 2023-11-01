// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/id.h>

#include <QObject>

#include <functional>

namespace Core {
class IEditor;
}

namespace TextEditor {
class TextEditorWidget;

namespace Internal { class TextEditorActionHandlerPrivate; }

// Redirects slots from global actions to the respective editor.

class TEXTEDITOR_EXPORT TextEditorActionHandler final
{
    TextEditorActionHandler(const TextEditorActionHandler &) = delete;
    TextEditorActionHandler &operator=(const TextEditorActionHandler &) = delete;

public:
    enum OptionalActionsMask {
        None = 0,
        Format = 1,
        UnCommentSelection = 2,
        UnCollapseAll = 4,
        FollowSymbolUnderCursor = 8,
        FollowTypeUnderCursor = 16,
        JumpToFileUnderCursor = 32,
        RenameSymbol = 64,
        FindUsage = 128,
        CallHierarchy = 256
    };
    using TextEditorWidgetResolver = std::function<TextEditorWidget *(Core::IEditor *)>;

    TextEditorActionHandler(Utils::Id editorId,
                            Utils::Id contextId,
                            uint optionalActions = None,
                            const TextEditorWidgetResolver &resolver = {});

    uint optionalActions() const;
    ~TextEditorActionHandler();

    void updateCurrentEditor();
    void updateActions();

    using Predicate = std::function<bool(Core::IEditor *editor)>;

    void setCanUndoCallback(const Predicate &callback);
    void setCanRedoCallback(const Predicate &callback);

    using UnhandledCallback = std::function<void(Utils::Id commandId, Core::IEditor *editor)>;
    void setUnhandledCallback(const UnhandledCallback &callback);

private:
    Internal::TextEditorActionHandlerPrivate *d;
};

} // namespace TextEditor
