/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CPPEDITOR_H
#define CPPEDITOR_H

#include "cppeditorenums.h"
#include <cplusplus/CppDocument.h>
#include <texteditor/basetexteditor.h>
#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

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

    int previousBlockState(QTextBlock block) const;
    QTextCursor moveToPreviousToken(QTextCursor::MoveMode mode) const;
    QTextCursor moveToNextToken(QTextCursor::MoveMode mode) const;

    void createToolBar(CPPEditorEditable *editable);

    int endOfNameUnderCursor();

    bool openEditorAt(CPlusPlus::Symbol *symbol);

    Core::ICore *m_core;
    CppTools::CppModelManagerInterface *m_modelManager;

    QList<int> m_contexts;
    QComboBox *m_methodCombo;
    CPlusPlus::OverviewModel *m_overviewModel;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITOR_H
