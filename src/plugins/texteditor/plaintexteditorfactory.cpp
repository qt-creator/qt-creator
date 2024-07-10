// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintexteditorfactory.h"
#include "basehoverhandler.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>

#include <utils/infobar.h>
#include <utils/qtcassert.h>

namespace TextEditor {

class PlainTextEditorWidget final : public TextEditorWidget
{
public:
    void finalizeInitialization() final
    {
        textDocument()->setMimeType(QLatin1String(Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
    }
};

class PlainTextEditorFactory final : public TextEditorFactory
{
public:
    PlainTextEditorFactory()
    {
        setId(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
        setDisplayName(::Core::Tr::tr(Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME));
        addMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
        addMimeType(QLatin1String("text/css")); // for some reason freedesktop thinks css is text/x-csrc
        addHoverHandler(new BaseHoverHandler);

        setDocumentCreator([]() { return new TextDocument(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID); });
        setEditorWidgetCreator([]() { return new PlainTextEditorWidget; });
        setUseGenericHighlighter(true);

        setOptionalActionMask(
                    OptionalActions::Format | OptionalActions::UnCommentSelection
                    | OptionalActions::UnCollapseAll);
    }
};

PlainTextEditorFactory &plainTextEditorFactory()
{
    static PlainTextEditorFactory thePlainTextEditorFactory;
    return thePlainTextEditorFactory;
}

BaseTextEditor *createPlainTextEditor()
{
    return qobject_cast<BaseTextEditor *>(plainTextEditorFactory().createEditor());
}

void Internal::setupPlainTextEditor()
{
    (void) plainTextEditorFactory(); // Trigger instantiation
}

} // namespace TextEditor
