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

#ifndef QMLJSEDITOR_H
#define QMLJSEDITOR_H

#include "qmljseditor_global.h"

#include <qmljs/qmljsscanner.h>
#include <qmljstools/qmljssemanticinfo.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/quickfix.h>
#include <texteditor/texteditorconstants.h>

#include <QSharedPointer>
#include <QModelIndex>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace Core {
class ICore;
}

namespace QmlJS {
    class ModelManagerInterface;
    class IContextPane;
    class LookupContext;
namespace AST {
    class UiObjectMember;
}
}

/*!
    The top-level namespace of the QmlJSEditor plug-in.
 */
namespace QmlJSEditor {
class QmlJSEditorEditable;
class FindReferences;

namespace Internal {
class QmlOutlineModel;
class SemanticInfoUpdater;
struct SemanticInfoUpdaterSource;
class SemanticHighlighter;
} // namespace Internal

struct QMLJSEDITOR_EXPORT Declaration
{
    QString text;
    int startLine;
    int startColumn;
    int endLine;
    int endColumn;

    Declaration()
        : startLine(0),
        startColumn(0),
        endLine(0),
        endColumn(0)
    { }
};

class QMLJSEDITOR_EXPORT QmlJSTextEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

    // used e.g. in qmljsprofiler
    Q_PROPERTY(QmlJSTools::SemanticInfo semanticInfo READ semanticInfo)

public:
    QmlJSTextEditorWidget(QWidget *parent = 0);
    ~QmlJSTextEditorWidget();

    virtual void unCommentSelection();

    QmlJSTools::SemanticInfo semanticInfo() const;
    bool isSemanticInfoOutdated() const;
    int editorRevision() const;

    Internal::QmlOutlineModel *outlineModel() const;
    QModelIndex outlineModelIndex();

    static QVector<TextEditor::TextStyle> highlighterFormatCategories();

    TextEditor::IAssistInterface *createAssistInterface(TextEditor::AssistKind assistKind,
                                                        TextEditor::AssistReason reason) const;

public slots:
    virtual void setTabSettings(const TextEditor::TabSettings &ts);
    void reparseDocument();
    void reparseDocumentNow();
    void updateSemanticInfo();
    void updateSemanticInfoNow();
    void findUsages();
    void renameUsages();
    void showContextPane();
    virtual void setFontSettings(const TextEditor::FontSettings &);

signals:
    void outlineModelIndexChanged(const QModelIndex &index);
    void selectedElementsChanged(QList<QmlJS::AST::UiObjectMember*> offsets,
                                 const QString &wordAtCursor);
    void semanticInfoUpdated();

private slots:
    void onDocumentUpdated(QmlJS::Document::Ptr doc);
    void modificationChanged(bool);

    void jumpToOutlineElement(int index);
    void updateOutlineNow();
    void updateOutlineIndexNow();
    void updateCursorPositionNow();
    void showTextMarker();
    void updateFileName();

    void updateUses();
    void updateUsesNow();

    void acceptNewSemanticInfo(const QmlJSTools::SemanticInfo &semanticInfo);
    void onCursorPositionChanged();
    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker);

    void performQuickFix(int index);

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    bool event(QEvent *e);
    void wheelEvent(QWheelEvent *event);
    void resizeEvent(QResizeEvent *event);
    void scrollContentsBy(int dx, int dy);
    TextEditor::BaseTextEditor *createEditor();
    void createToolBar(QmlJSEditorEditable *editable);
    TextEditor::BaseTextEditorWidget::Link findLinkAt(const QTextCursor &cursor, bool resolveTarget = true);
    QString foldReplacementText(const QTextBlock &block) const;

private:
    bool isClosingBrace(const QList<QmlJS::Token> &tokens) const;

    void setSelectedElements();
    QString wordUnderCursor() const;

    QModelIndex indexForPosition(unsigned cursorPosition, const QModelIndex &rootIndex = QModelIndex()) const;
    bool hideContextPane();

    const Core::Context m_context;

    QTimer *m_updateDocumentTimer;
    QTimer *m_updateUsesTimer;
    QTimer *m_updateSemanticInfoTimer;
    QTimer *m_updateOutlineTimer;
    QTimer *m_updateOutlineIndexTimer;
    QTimer *m_cursorPositionTimer;
    QComboBox *m_outlineCombo;
    Internal::QmlOutlineModel *m_outlineModel;
    QModelIndex m_outlineModelIndex;
    QmlJS::ModelManagerInterface *m_modelManager;
    QTextCharFormat m_occurrencesFormat;
    QTextCharFormat m_occurrencesUnusedFormat;
    QTextCharFormat m_occurrenceRenameFormat;

    Internal::SemanticInfoUpdater *m_semanticInfoUpdater;
    QmlJSTools::SemanticInfo m_semanticInfo;
    int m_futureSemanticInfoRevision;

    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;

    QmlJS::IContextPane *m_contextPane;
    int m_oldCursorPosition;

    FindReferences *m_findReferences;
    Internal::SemanticHighlighter *m_semanticHighlighter;

    friend class Internal::SemanticHighlighter;
};

} // namespace QmlJSEditor

#endif // QMLJSEDITOR_H
