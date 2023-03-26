// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourceagent.h"

#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggerinternalconstants.h"
#include "debuggertr.h"
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

namespace Debugger::Internal {

class SourceAgentPrivate
{
public:
    SourceAgentPrivate();
    ~SourceAgentPrivate();

public:
    QPointer<BaseTextEditor> editor;
    QPointer<DebuggerEngine> engine;
    TextMark *locationMark = nullptr;
    QString path;
    QString producer;
};

SourceAgentPrivate::SourceAgentPrivate()
  : producer("remote")
{
}

SourceAgentPrivate::~SourceAgentPrivate()
{
    if (editor)
        EditorManager::closeDocuments({editor->document()});
    editor = nullptr;
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
        QString titlePattern = d->producer + ": "
            + Utils::FilePath::fromString(filePath).fileName();
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
    d->locationMark = nullptr;
    if (d->engine->stackHandler()->currentFrame().file == Utils::FilePath::fromString(d->path)) {
        int lineNumber = d->engine->stackHandler()->currentFrame().line;

        d->locationMark = new TextMark(Utils::FilePath(),
                                       lineNumber,
                                       {Tr::tr("Debugger Location"),
                                        Constants::TEXT_MARK_CATEGORY_LOCATION});
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

} // Debugger::Internal
