/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppeditorenums.h"

#include <cplusplus/CppDocument.h>
#include <texteditor/basetexteditor.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace CPlusPlus {
class OverviewModel;
class Symbol;
}

namespace CppTools {
class CppModelManagerInterface;
}

namespace TextEditor {
class FontSettings;
}

namespace CppEditor {
namespace Internal {

class CPPEditor;

class CPPEditorEditable : public TextEditor::BaseTextEditorEditable
{
    Q_OBJECT
public:
    CPPEditorEditable(CPPEditor *);
    QList<int> context() const;

    bool duplicateSupported() const { return true; }
    Core::IEditor *duplicate(QWidget *parent);
    const char *kind() const;

private:
    QList<int> m_context;
};

class CPPEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    typedef TextEditor::TabSettings TabSettings;

    CPPEditor(QWidget *parent);
    ~CPPEditor();

    void unCommentSelection();

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);
    void switchDeclarationDefinition();
    void jumpToDefinition();

    void moveToPreviousToken();
    void moveToNextToken();

    void deleteStartOfToken();
    void deleteEndOfToken();

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    TextEditor::BaseTextEditorEditable *createEditableInterface();

    // Rertuns true if key triggers anindent.
    virtual bool isElectricCharacter(const QChar &ch) const;

private slots:
    void updateFileName();
    void jumpToMethod(int index);
    void updateMethodBoxIndex();
    void updateMethodBoxToolTip();
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);

private:
    CPlusPlus::Symbol *findDefinition(CPlusPlus::Symbol *symbol);
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);

    TextEditor::ITextEditor *openCppEditorAt(const QString &fileName, int line,
                                             int column = 0);

    int previousBlockState(QTextBlock block) const;
    QTextCursor moveToPreviousToken(QTextCursor::MoveMode mode) const;
    QTextCursor moveToNextToken(QTextCursor::MoveMode mode) const;

    void createToolBar(CPPEditorEditable *editable);

    int endOfNameUnderCursor();

    bool openEditorAt(CPlusPlus::Symbol *symbol);

    CppTools::CppModelManagerInterface *m_modelManager;

    QList<int> m_contexts;
    QComboBox *m_methodCombo;
    CPlusPlus::OverviewModel *m_overviewModel;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
