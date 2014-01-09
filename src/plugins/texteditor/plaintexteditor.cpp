/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "plaintexteditor.h"
#include "tabsettings.h"
#include "texteditorplugin.h"
#include "texteditorsettings.h"
#include "basetextdocument.h"
#include "normalindenter.h"
#include "highlighterutils.h"
#include <texteditor/generichighlighter/context.h>
#include <texteditor/generichighlighter/highlightdefinition.h>
#include <texteditor/generichighlighter/highlighter.h>
#include <texteditor/generichighlighter/highlightersettings.h>
#include <texteditor/generichighlighter/manager.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <QSharedPointer>

using namespace Core;
using namespace TextEditor::Internal;

namespace TextEditor {

PlainTextEditor::PlainTextEditor(PlainTextEditorWidget *editor)
  : BaseTextEditor(editor)
{
    setContext(Core::Context(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                             TextEditor::Constants::C_TEXTEDITOR));
}

PlainTextEditorWidget::PlainTextEditorWidget(QWidget *parent)
  : BaseTextEditorWidget(parent)
{
    ctor();
}

PlainTextEditorWidget::PlainTextEditorWidget(BaseTextDocument *doc, QWidget *parent)
    : BaseTextEditorWidget(doc, parent)
{
    ctor();
}

PlainTextEditorWidget::PlainTextEditorWidget(PlainTextEditorWidget *other)
    : BaseTextEditorWidget(other)
{
    ctor();
}

void PlainTextEditorWidget::ctor()
{
    m_isMissingSyntaxDefinition = false;
    setRevisionsVisible(true);
    setMarksVisible(true);
    setLineSeparatorsAllowed(true);
    setIndenter(new NormalIndenter); // Currently only "normal" indentation is supported.

    setMimeType(QLatin1String(TextEditor::Constants::C_TEXTEDITOR_MIMETYPE_TEXT));

    m_commentDefinition.clearCommentStyles();

    // If configure() is called immediately the whole document is considered modified
    connect(baseTextDocument(), SIGNAL(changed()), this, SLOT(configure()), Qt::QueuedConnection);
    connect(Manager::instance(), SIGNAL(mimeTypesRegistered()), this, SLOT(configure()));
}

IEditor *PlainTextEditor::duplicate()
{
    PlainTextEditorWidget *newWidget = new PlainTextEditorWidget(
                qobject_cast<PlainTextEditorWidget *>(editorWidget()));
    TextEditorSettings::initializeEditor(newWidget);
    return newWidget->editor();
}

Id PlainTextEditor::id() const
{
    return Core::Constants::K_DEFAULT_TEXT_EDITOR_ID;
}

void PlainTextEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

void PlainTextEditorWidget::setTabSettings(const TextEditor::TabSettings &ts)
{
    BaseTextEditorWidget::setTabSettings(ts);

    if (baseTextDocument()->syntaxHighlighter()) {
        Highlighter *highlighter =
            static_cast<Highlighter *>(baseTextDocument()->syntaxHighlighter());
        highlighter->setTabSettings(ts);
    }
}

void PlainTextEditorWidget::configure()
{
    MimeType mimeType;
    if (baseTextDocument())
        mimeType = MimeDatabase::findByFile(baseTextDocument()->filePath());
    configure(mimeType);
}

void PlainTextEditorWidget::configure(const QString &mimeType)
{
    configure(MimeDatabase::findByType(mimeType));
}

void PlainTextEditorWidget::configure(const MimeType &mimeType)
{
    Highlighter *highlighter = new Highlighter();
    baseTextDocument()->setSyntaxHighlighter(highlighter);

    setCodeFoldingSupported(false);

    if (!mimeType.isNull()) {
        m_isMissingSyntaxDefinition = true;

        setMimeTypeForHighlighter(highlighter, mimeType);
        const QString &type = mimeType.type();
        setMimeType(type);

        QString definitionId = Manager::instance()->definitionIdByMimeType(type);
        if (definitionId.isEmpty())
            definitionId = findDefinitionId(mimeType, true);

        if (!definitionId.isEmpty()) {
            m_isMissingSyntaxDefinition = false;
            const QSharedPointer<HighlightDefinition> &definition =
                Manager::instance()->definition(definitionId);
            if (!definition.isNull() && definition->isValid()) {
                m_commentDefinition.isAfterWhiteSpaces = definition->isCommentAfterWhiteSpaces();
                m_commentDefinition.singleLine = definition->singleLineComment();
                m_commentDefinition.multiLineStart = definition->multiLineCommentStart();
                m_commentDefinition.multiLineEnd = definition->multiLineCommentEnd();

                setCodeFoldingSupported(true);
            }
        } else if (baseTextDocument()) {
            const QString &fileName = baseTextDocument()->filePath();
            if (TextEditorSettings::highlighterSettings().isIgnoredFilePattern(fileName))
                m_isMissingSyntaxDefinition = false;
        }
    }

    setFontSettings(TextEditorSettings::fontSettings());

    emit configured(editor());
}

bool PlainTextEditorWidget::isMissingSyntaxDefinition() const
{
    return m_isMissingSyntaxDefinition;
}

void PlainTextEditorWidget::acceptMissingSyntaxDefinitionInfo()
{
    ICore::showOptionsDialog(Constants::TEXT_EDITOR_SETTINGS_CATEGORY,
                             Constants::TEXT_EDITOR_HIGHLIGHTER_SETTINGS);
}

} // namespace TextEditor
