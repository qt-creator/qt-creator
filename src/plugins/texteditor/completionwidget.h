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

#ifndef COMPLETIONWIDGET_H
#define COMPLETIONWIDGET_H

#include <QtGui/QListView>
#include <QPointer>

namespace TextEditor {

struct CompletionItem;
class ITextEditable;

namespace Internal {

class AutoCompletionModel;
class CompletionSupport;
class CompletionListView;

/* The completion widget is responsible for showing a list of possible completions.
   It is only used by the CompletionSupport.
 */
class CompletionWidget : public QFrame
{
    Q_OBJECT

public:
    CompletionWidget(CompletionSupport *support, ITextEditable *editor);
    ~CompletionWidget();

    void setQuickFix(bool quickFix);
    void setCompletionItems(const QList<TextEditor::CompletionItem> &completionitems);
    void showCompletions(int startPos);

signals:
    void itemSelected(const TextEditor::CompletionItem &item);
    void completionListClosed();

public slots:
    void closeList(const QModelIndex &index = QModelIndex());

private:
    void updatePositionAndSize(int startPos);

private:
    CompletionSupport *m_support;
    ITextEditable *m_editor;
    CompletionListView *m_completionListView;
};

class CompletionListView : public QListView
{
    Q_OBJECT

public:
    ~CompletionListView();

signals:
    void itemSelected(const TextEditor::CompletionItem &item);
    void completionListClosed();

protected:
    bool event(QEvent *e);

private:
    friend class CompletionWidget;

    CompletionListView(CompletionSupport *support, ITextEditable *editor, CompletionWidget *completionWidget);

    void setQuickFix(bool quickFix);
    void setCompletionItems(const QList<TextEditor::CompletionItem> &completionitems);
    void keyboardSearch(const QString &search);
    void closeList(const QModelIndex &index);

    bool m_blockFocusOut;
    bool m_quickFix;
    ITextEditable *m_editor;
    QWidget *m_editorWidget;
    CompletionWidget *m_completionWidget;
    AutoCompletionModel *m_model;
    CompletionSupport *m_support;
};

} // namespace Internal
} // namespace TextEditor

#endif // COMPLETIONWIDGET_H

