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
#include <QTextLayout>
#include <QVector>

QT_BEGIN_NAMESPACE
class QComboBox;
class QTimer;
QT_END_NAMESPACE

namespace Core { class ICore; }

namespace QmlJS {
    class ModelManagerInterface;
    class IContextPane;
    class LookupContext;
namespace AST { class UiObjectMember; }
}

/*!
    The top-level namespace of the QmlJSEditor plug-in.
 */
namespace QmlJSEditor {
class QmlJSEditorDocument;
class FindReferences;

namespace Internal {

class QmlJSEditor;
class QmlOutlineModel;

class QmlJSTextEditorWidget : public TextEditor::BaseTextEditorWidget
{
    Q_OBJECT

public:
    QmlJSTextEditorWidget(QWidget *parent = 0);
    QmlJSTextEditorWidget(QmlJSTextEditorWidget *other);
    ~QmlJSTextEditorWidget();

    QmlJSEditorDocument *qmlJsEditorDocument() const;

    QModelIndex outlineModelIndex();

    TextEditor::IAssistInterface *createAssistInterface(TextEditor::AssistKind assistKind,
                                                        TextEditor::AssistReason reason) const;
public slots:
    void findUsages();
    void renameUsages();
    void showContextPane();

signals:
    void outlineModelIndexChanged(const QModelIndex &index);
    void selectedElementsChanged(QList<QmlJS::AST::UiObjectMember*> offsets,
                                 const QString &wordAtCursor);
private slots:
    void modificationChanged(bool);

    void jumpToOutlineElement(int index);
    void updateOutlineIndexNow();
    void updateContextPane();
    void showTextMarker();

    void updateUses();

    void semanticInfoUpdated(const QmlJSTools::SemanticInfo &semanticInfo);
    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker);

    void performQuickFix(int index);
    void updateCodeWarnings(QmlJS::Document::Ptr doc);

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    bool event(QEvent *e);
    void wheelEvent(QWheelEvent *event);
    void resizeEvent(QResizeEvent *event);
    void scrollContentsBy(int dx, int dy);
    void applyFontSettings();
    TextEditor::BaseTextEditor *createEditor();
    void createToolBar(QmlJSEditor *editable);
    TextEditor::BaseTextEditorWidget::Link findLinkAt(const QTextCursor &cursor,
                                                      bool resolveTarget = true,
                                                      bool inNextSplit = false);
    QString foldReplacementText(const QTextBlock &block) const;

private:
    QmlJSTextEditorWidget(TextEditor::BaseTextEditorWidget *); // avoid stupidity
    void ctor();
    bool isClosingBrace(const QList<QmlJS::Token> &tokens) const;

    void setSelectedElements();
    QString wordUnderCursor() const;

    QModelIndex indexForPosition(unsigned cursorPosition, const QModelIndex &rootIndex = QModelIndex()) const;
    bool hideContextPane();

    QmlJSEditorDocument *m_qmlJsEditorDocument;
    QTimer *m_updateUsesTimer; // to wait for multiple text cursor position changes
    QTimer *m_updateOutlineIndexTimer;
    QTimer *m_contextPaneTimer;
    QComboBox *m_outlineCombo;
    QModelIndex m_outlineModelIndex;
    QmlJS::ModelManagerInterface *m_modelManager;

    QList<TextEditor::QuickFixOperation::Ptr> m_quickFixes;

    QmlJS::IContextPane *m_contextPane;
    int m_oldCursorPosition;

    FindReferences *m_findReferences;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSEDITOR_H
