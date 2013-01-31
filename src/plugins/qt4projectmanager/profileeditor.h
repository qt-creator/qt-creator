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

#ifndef PROFILEEDITOR_H
#define PROFILEEDITOR_H

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <utils/uncommentselection.h>

namespace TextEditor {
class FontSettings;
class TextEditorActionHandler;
}

namespace Qt4ProjectManager {
namespace Internal {

class ProFileEditorFactory;
class ProFileEditorWidget;

class ProFileEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    ProFileEditor(ProFileEditorWidget *);

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    Core::Id id() const;
    bool isTemporary() const { return false; }
};

class ProFileEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    ProFileEditorWidget(QWidget *parent, ProFileEditorFactory *factory,
                  TextEditor::TextEditorActionHandler *ah);

    bool save(const QString &fileName = QString());

    ProFileEditorFactory *factory() { return m_factory; }
    TextEditor::TextEditorActionHandler *actionHandler() const { return m_ah; }

    void unCommentSelection();

protected:
    virtual Link findLinkAt(const QTextCursor &, bool resolveTarget = true);
    TextEditor::BaseTextEditor *createEditor();
    void contextMenuEvent(QContextMenuEvent *);

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);

private:
    ProFileEditorFactory *m_factory;
    TextEditor::TextEditorActionHandler *m_ah;
    Utils::CommentDefinition m_commentDefinition;
};

class ProFileDocument : public TextEditor::BaseTextDocument
{
    Q_OBJECT

public:
    ProFileDocument();
    QString defaultPath() const;
    QString suggestedFileName() const;

    // qmake project files doesn't support UTF8-BOM
    // If the BOM would be added qmake would fail and QtCreator couldn't parse the project file
    bool supportsUtf8Bom() { return false; }
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // PROFILEEDITOR_H
