/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "sourceagent.h"

#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "stackframe.h"
#include "stackhandler.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <cppeditor/cppeditorconstants.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QMetaObject>
#include <QTimer>
#include <QPointer>
#include <QFileInfo>

#include <QMessageBox>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>
#include <QIcon>

#include <limits.h>

using namespace Core;

namespace Debugger {
namespace Internal {

class SourceAgentPrivate
{
public:
    SourceAgentPrivate();
    ~SourceAgentPrivate();

public:
    QPointer<TextEditor::ITextEditor> editor;
    QPointer<DebuggerEngine> engine;
    TextEditor::ITextMark *locationMark;
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
    if (editor) {
        EditorManager *editorManager = EditorManager::instance();
        editorManager->closeEditors(QList<IEditor *>() << editor);
    }
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
    using namespace Core;
    using namespace TextEditor;

    d->path = filePath;

    if (!d->editor) {
        QString titlePattern = d->producer + QLatin1String(": ")
            + QFileInfo(filePath).fileName();
        d->editor = qobject_cast<ITextEditor *>(
            EditorManager::openEditorWithContents(
                CppEditor::Constants::CPPEDITOR_ID,
                &titlePattern, content));
        QTC_ASSERT(d->editor, return);
        d->editor->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);

        BaseTextEditorWidget *baseTextEdit =
                qobject_cast<BaseTextEditorWidget *>(d->editor->widget());
        if (baseTextEdit)
            baseTextEdit->setRequestMarkEnabled(true);
    }

    EditorManager::activateEditor(d->editor);

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return);
    plainTextEdit->setReadOnly(true);

    updateLocationMarker();
}

void SourceAgent::updateLocationMarker()
{
    QTC_ASSERT(d->editor, return);

    if (d->locationMark)
        d->editor->markableInterface()->removeMark(d->locationMark);
    delete d->locationMark;
    d->locationMark = 0;
    if (d->engine->stackHandler()->currentFrame().file == d->path) {
        int lineNumber = d->engine->stackHandler()->currentFrame().line;
        d->locationMark = new TextEditor::ITextMark(lineNumber);
        d->locationMark->setIcon(debuggerCore()->locationMarkIcon());
        d->locationMark->setPriority(TextEditor::ITextMark::HighPriority);
        d->editor->markableInterface()->addMark(d->locationMark);
        QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit *>(d->editor->widget());
        QTC_ASSERT(plainTextEdit, return);
        QTextCursor tc = plainTextEdit->textCursor();
        QTextBlock block = tc.document()->findBlockByNumber(lineNumber - 1);
        tc.setPosition(block.position());
        plainTextEdit->setTextCursor(tc);
        EditorManager::activateEditor(d->editor);
    }
}

} // namespace Internal
} // namespace Debugger
