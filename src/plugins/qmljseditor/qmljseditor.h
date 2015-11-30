/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSEDITOR_H
#define QMLJSEDITOR_H

#include "qmljseditor_global.h"

#include <qmljs/qmljsscanner.h>
#include <qmljstools/qmljssemanticinfo.h>
#include <texteditor/texteditor.h>
#include <texteditor/quickfix.h>
#include <texteditor/texteditorconstants.h>
#include <utils/uncommentselection.h>

#include <QModelIndex>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace QmlJS {
    class ModelManagerInterface;
    class IContextPane;
namespace AST { class UiObjectMember; }
}

namespace QmlJSEditor {

class QmlJSEditorDocument;
class FindReferences;

namespace Internal {

class QmlJSEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT

public:
    QmlJSEditorWidget();

    void finalizeInitialization() override;

    QmlJSEditorDocument *qmlJsEditorDocument() const;

    QModelIndex outlineModelIndex();

    TextEditor::AssistInterface *createAssistInterface(TextEditor::AssistKind assistKind,
                           TextEditor::AssistReason reason) const override;

    void inspectElementUnderCursor() const;

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

    void performQuickFix(int index);
    void updateCodeWarnings(QmlJS::Document::Ptr doc);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    bool event(QEvent *e) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void applyFontSettings() override;
    void createToolBar();
    TextEditor::TextEditorWidget::Link findLinkAt(const QTextCursor &cursor,
                                                      bool resolveTarget = true,
                                                      bool inNextSplit = false) override;
    QString foldReplacementText(const QTextBlock &block) const override;
    void onRefactorMarkerClicked(const TextEditor::RefactorMarker &marker) override;

private:
    bool isClosingBrace(const QList<QmlJS::Token> &tokens) const;

    void setSelectedElements();
    QString wordUnderCursor() const;

    QModelIndex indexForPosition(unsigned cursorPosition, const QModelIndex &rootIndex = QModelIndex()) const;
    bool hideContextPane();

    QmlJSEditorDocument *m_qmlJsEditorDocument;
    QTimer m_updateUsesTimer; // to wait for multiple text cursor position changes
    QTimer m_updateOutlineIndexTimer;
    QTimer m_contextPaneTimer;
    QComboBox *m_outlineCombo;
    QModelIndex m_outlineModelIndex;
    QmlJS::ModelManagerInterface *m_modelManager;

    TextEditor::QuickFixOperations m_quickFixes;

    QmlJS::IContextPane *m_contextPane;
    int m_oldCursorPosition;

    FindReferences *m_findReferences;
};


class QmlJSEditor : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    QmlJSEditor();

    bool isDesignModePreferred() const override;
};

class QmlJSEditorFactory : public TextEditor::TextEditorFactory
{
    Q_OBJECT

public:
    QmlJSEditorFactory();
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSEDITOR_H
