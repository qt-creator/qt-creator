/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sourceagent.h"

#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
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
  , locationMark(0)
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
    d = 0;
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
        d->locationMark->setIcon(locationMarkIcon());
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
