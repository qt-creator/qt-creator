/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljsrefactoringchanges.h"
#include "qmljseditorcodeformatter.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>

using namespace QmlJS;
using namespace QmlJSEditor;

QmlJSRefactoringChanges::QmlJSRefactoringChanges(ModelManagerInterface *modelManager,
                                                 const Snapshot &snapshot)
    : m_modelManager(modelManager)
    , m_snapshot(snapshot)
{
    Q_ASSERT(modelManager);
}

void QmlJSRefactoringChanges::indentSelection(const QTextCursor &selection) const
{
    // ### shares code with QmlJSTextEditor::indent
    QTextDocument *doc = selection.document();

    QTextBlock block = doc->findBlock(selection.selectionStart());
    const QTextBlock end = doc->findBlock(selection.selectionEnd()).next();

    const TextEditor::TabSettings &tabSettings(TextEditor::TextEditorSettings::instance()->tabSettings());
    QtStyleCodeFormatter codeFormatter(tabSettings);
    codeFormatter.updateStateUntil(block);

    do {
        tabSettings.indentLine(block, codeFormatter.indentFor(block));
        codeFormatter.updateLineStateChange(block);
        block = block.next();
    } while (block.isValid() && block != end);
}

void QmlJSRefactoringChanges::fileChanged(const QString &fileName)
{
    m_modelManager->updateSourceFiles(QStringList(fileName), true);
}
