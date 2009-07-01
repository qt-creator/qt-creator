/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppeditorenums.h"

#include <cplusplus/CppDocument.h>
#include <texteditor/basetexteditor.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSortFilterProxyModel;
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

    bool isTemporary() const { return false; }

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

    void indentInsertedText(const QTextCursor &tc);

public slots:
    virtual void setFontSettings(const TextEditor::FontSettings &);
    virtual void setDisplaySettings(const TextEditor::DisplaySettings &);
    void setSortedMethodOverview(bool sort);
    void switchDeclarationDefinition();
    void jumpToDefinition();

    void moveToPreviousToken();
    void moveToNextToken();

    void deleteStartOfToken();
    void deleteEndOfToken();

protected:
    bool event(QEvent *e);
    void contextMenuEvent(QContextMenuEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void leaveEvent(QEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void keyPressEvent(QKeyEvent *);

    TextEditor::BaseTextEditorEditable *createEditableInterface();

    // Rertuns true if key triggers anindent.
    virtual bool isElectricCharacter(const QChar &ch) const;

private slots:
    void updateFileName();
    void jumpToMethod(int index);
    void updateMethodBoxIndex();
    void updateMethodBoxIndexNow();
    void updateMethodBoxToolTip();
    void updateUses();
    void updateUsesNow();
    void onDocumentUpdated(CPlusPlus::Document::Ptr doc);
    void reformatDocument();
    void simplifyDeclarations();
    void renameInPlace();
    void onContentsChanged(int position, int charsRemoved, int charsAdded);

private:
    bool sortedMethodOverview() const;
    CPlusPlus::Symbol *findDefinition(CPlusPlus::Symbol *symbol);
    virtual void indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar);

    TextEditor::ITextEditor *openCppEditorAt(const QString &fileName, int line,
                                             int column = 0);

    int previousBlockState(QTextBlock block) const;
    QTextCursor moveToPreviousToken(QTextCursor::MoveMode mode) const;
    QTextCursor moveToNextToken(QTextCursor::MoveMode mode) const;

    void createToolBar(CPPEditorEditable *editable);

    void abortRename();

    struct Link
    {
        Link(const QString &fileName = QString(),
             int line = 0,
             int column = 0)
            : pos(-1)
            , length(-1)
            , fileName(fileName)
            , line(line)
            , column(column)
        {}

        int pos;           // Link position
        int length;        // Link length

        QString fileName;  // Target file
        int line;          // Target line
        int column;        // Target column
    };

    void showLink(const Link &);
    void clearLink();

    Link findLinkAt(const QTextCursor &, bool lookupDefinition = true);
    static Link linkToSymbol(CPlusPlus::Symbol *symbol);
    bool openCppEditorAt(const Link &);

    bool m_mouseNavigationEnabled;
    bool m_showingLink;
    QTextCharFormat m_linkFormat;

    CppTools::CppModelManagerInterface *m_modelManager;

    QList<int> m_contexts;
    QComboBox *m_methodCombo;
    CPlusPlus::OverviewModel *m_overviewModel;
    QSortFilterProxyModel *m_proxyModel;
    QAction *m_sortAction;
    QTimer *m_updateMethodBoxTimer;
    QTimer *m_updateUsesTimer;

    QList<QTextEdit::ExtraSelection> m_renameSelections;
    int m_currentRenameSelection;
    bool m_inRename;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
