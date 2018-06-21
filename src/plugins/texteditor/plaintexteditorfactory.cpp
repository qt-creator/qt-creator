/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "plaintexteditorfactory.h"
#include "texteditor.h"
#include "textdocument.h"
#include "normalindenter.h"
#include "texteditoractionhandler.h"
#include "texteditorconstants.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"
#include "basehoverhandler.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/infobar.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>

namespace TextEditor {

static PlainTextEditorFactory *m_instance = 0;

class PlainTextEditorWidget : public TextEditorWidget
{
public:
    PlainTextEditorWidget() {}
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
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", Core::Constants::K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME));
    addMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));
    addMimeType(QLatin1String("text/css")); // for some reason freedesktop thinks css is text/x-csrc
    addHoverHandler(new BaseHoverHandler);

    setDocumentCreator([]() { return new TextDocument(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID); });
    setEditorWidgetCreator([]() { return new PlainTextEditorWidget; });
    setIndenterCreator([]() { return new NormalIndenter; });
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
