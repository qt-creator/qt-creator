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

#include "sourceagent.h"

#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggerinternalconstants.h"
#include "stackhandler.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/textmark.h>

#include <cppeditor/cppeditorconstants.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextBlock>

#include <limits.h>

using namespace Core;
using namespace TextEditor;

namespace Debugger {
namespace Internal {

class SourceAgentPrivate
{
public:
    SourceAgentPrivate();
    ~SourceAgentPrivate();

public:
    QPointer<BaseTextEditor> editor;
    QPointer<DebuggerEngine> engine;
    TextMark *locationMark;
    QString path;
    QString producer;
};

SourceAgentPrivate::SourceAgentPrivate()
  : editor(0)
  , locationMark(nullptr)
  , producer(QLatin1String("remote"))
{
}

SourceAgentPrivate::~SourceAgentPrivate()
{
    if (editor)
        EditorManager::closeDocument(editor->document());
    editor = 0;
    delete locationMark;
}

SourceAgent::SourceAgent(DebuggerEngine *engine)
    : d(new SourceAgentPrivate)
{
    d->engine = engine;
}

SourceAgent::~SourceAgent()
{
    delete d;
    d = nullptr;
}

void SourceAgent::setSourceProducerName(const QString &name)
{
    d->producer = name;
}

void SourceAgent::setContent(const QString &filePath, const QString &content)
{
    QTC_ASSERT(d, return);
    d->path = filePath;

    if (!d->editor) {
        QString titlePattern = d->producer + QLatin1String(": ")
            + Utils::FileName::fromString(filePath).fileName();
        d->editor = qobject_cast<BaseTextEditor *>(
            EditorManager::openEditorWithContents(
                CppEditor::Constants::CPPEDITOR_ID,
                &titlePattern, content.toUtf8()));
        QTC_ASSERT(d->editor, return);
        d->editor->document()->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);

        TextEditorWidget *baseTextEdit = d->editor->editorWidget();
        if (baseTextEdit)
            baseTextEdit->setRequestMarkEnabled(true);
    } else {
        EditorManager::activateEditor(d->editor);
    }

    QPlainTextEdit *plainTextEdit = d->editor->editorWidget();
    QTC_ASSERT(plainTextEdit, return);
    plainTextEdit->setReadOnly(true);

    updateLocationMarker();
}

void SourceAgent::updateLocationMarker()
{
    QTC_ASSERT(d->editor, return);

    if (d->locationMark)
        d->editor->textDocument()->removeMark(d->locationMark);
    delete d->locationMark;
    d->locationMark = 0;
    if (d->engine->stackHandler()->currentFrame().file == d->path) {
        int lineNumber = d->engine->stackHandler()->currentFrame().line;

        d->locationMark = new TextMark(QString(), lineNumber,
                                       Constants::TEXT_MARK_CATEGORY_LOCATION);
        d->locationMark->setIcon(Icons::LOCATION.icon());
        d->locationMark->setPriority(TextMark::HighPriority);

        d->editor->textDocument()->addMark(d->locationMark);
        QTextCursor tc = d->editor->textCursor();
        QTextBlock block = tc.document()->findBlockByNumber(lineNumber - 1);
        tc.setPosition(block.position());
        d->editor->setTextCursor(tc);
        EditorManager::activateEditor(d->editor);
    }
}

} // namespace Internal
} // namespace Debugger
