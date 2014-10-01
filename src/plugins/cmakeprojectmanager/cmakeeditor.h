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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CMAKEEDITOR_H
#define CMAKEEDITOR_H

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <utils/uncommentselection.h>


namespace TextEditor { class FontSettings; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeEditorWidget;
class CMakeHighlighter;
class CMakeManager;

class CMakeEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    CMakeEditor(CMakeEditorWidget *);

    Core::IEditor *duplicate();
    TextEditor::CompletionAssistProvider *completionAssistProvider();

    QString contextHelpId() const;

private slots:
    void markAsChanged();
    void build();
};

class CMakeEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    CMakeEditorWidget(QWidget *parent = 0);
    CMakeEditorWidget(CMakeEditorWidget *other);

    bool save(const QString &fileName = QString());

    Link findLinkAt(const QTextCursor &cursor, bool resolveTarget = true, bool inNextSplit = false);

protected:
    TextEditor::BaseTextEditor *createEditor();
    void contextMenuEvent(QContextMenuEvent *e);

public slots:
    void unCommentSelection();

private:
    CMakeEditorWidget(TextEditor::BaseTextEditorWidget *); // avoid stupidity
    void ctor();
    Utils::CommentDefinition m_commentDefinition;
};

class CMakeDocument : public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    CMakeDocument();
    QString defaultPath() const;
    QString suggestedFileName() const;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEEDITOR_H
