// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintexteditorfactory.h"
#include "basehoverhandler.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditoractionhandler.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"
#include "textindenter.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <utils/infobar.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace TextEditor {

static PlainTextEditorFactory *m_instance = nullptr;

class PlainTextEditorWidget : public TextEditorWidget
{
public:
    PlainTextEditorWidget() = default;
    void finalizeInitialization() override
    {
        textDocument()->setMimeType(QLatin1String(Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
    }
};

PlainTextEditorFactory::PlainTextEditorFactory()
{
    QTC_CHECK(!m_instance);
    m_instance = this;
    setId(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    setDisplayName(::Core::Tr::tr(Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME));
    addMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
    addMimeType(QLatin1String("text/css")); // for some reason freedesktop thinks css is text/x-csrc
    addHoverHandler(new BaseHoverHandler);

    setDocumentCreator([]() { return new TextDocument(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID); });
    setEditorWidgetCreator([]() { return new PlainTextEditorWidget; });
    setUseGenericHighlighter(true);

    setEditorActionHandlers(TextEditorActionHandler::Format |
                            TextEditorActionHandler::UnCommentSelection |
                            TextEditorActionHandler::UnCollapseAll);
}

PlainTextEditorFactory *PlainTextEditorFactory::instance()
{
    return m_instance;
}

BaseTextEditor *PlainTextEditorFactory::createPlainTextEditor()
{
    return qobject_cast<BaseTextEditor *>(m_instance->createEditor());
}

} // namespace TextEditor
