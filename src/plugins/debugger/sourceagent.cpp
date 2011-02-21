/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "sourceagent.h"

#include "breakhandler.h"
#include "debuggerengine.h"
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

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QtCore/QFileInfo>

#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>
#include <QtGui/QIcon>

#include <limits.h>

using namespace Core;

namespace Debugger {
namespace Internal {

class LocationMarkFoo : public TextEditor::ITextMark
{
public:
    LocationMarkFoo() {}

    QIcon icon() const { return debuggerCore()->locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
    void documentClosing() {}
};

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
  , locationMark(new LocationMarkFoo)
  , producer("remote")
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
    : QObject(0), d(new SourceAgentPrivate)
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

    EditorManager *editorManager = EditorManager::instance();
    if (!d->editor) {
        QString titlePattern = d->producer + QLatin1String(": ")
            + QFileInfo(filePath).fileName();
        d->editor = qobject_cast<ITextEditor *>(
            editorManager->openEditorWithContents(
                CppEditor::Constants::CPPEDITOR_ID,
                &titlePattern, content));
        QTC_ASSERT(d->editor, return);
        d->editor->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);

        BaseTextEditorWidget *baseTextEdit =
                qobject_cast<BaseTextEditorWidget *>(d->editor->widget());
        if (baseTextEdit)
            baseTextEdit->setRequestMarkEnabled(true);
    }

    editorManager->activateEditor(d->editor);

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return);
    plainTextEdit->setReadOnly(true);

    updateLocationMarker();
}

void SourceAgent::updateLocationMarker()
{
    QTC_ASSERT(d->editor, return);

    d->editor->markableInterface()->removeMark(d->locationMark);
    if (d->engine->stackHandler()->currentFrame().file == d->path) {
        int lineNumber = d->engine->stackHandler()->currentFrame().line;
        d->editor->markableInterface()->addMark(d->locationMark, lineNumber);
        QPlainTextEdit *plainTextEdit = qobject_cast<QPlainTextEdit *>(d->editor->widget());
        QTC_ASSERT(plainTextEdit, return);
        QTextCursor tc = plainTextEdit->textCursor();
        QTextBlock block = tc.document()->findBlockByNumber(lineNumber - 1);
        tc.setPosition(block.position());
        plainTextEdit->setTextCursor(tc);
        EditorManager *editorManager = EditorManager::instance();
        editorManager->activateEditor(d->editor);
    }
}

} // namespace Internal
} // namespace Debugger
