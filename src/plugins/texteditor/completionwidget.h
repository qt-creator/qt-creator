/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef COMPLETIONWIDGET_H
#define COMPLETIONWIDGET_H

#include <QtGui/QListView>
#include <QtCore/QPointer>
#include <QtCore/QTimer>

namespace TextEditor {

class CompletionItem;
class ITextEditable;
class CompletionSupport;

namespace Internal {

class AutoCompletionModel;
class CompletionListView;
class CompletionInfoFrame;

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

    QChar typedChar() const;
    CompletionItem currentCompletionItem() const;

    void setCurrentIndex(int index);
    bool explicitlySelected() const;

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

    CompletionItem currentCompletionItem() const;
    bool explicitlySelected() const;

signals:
    void itemSelected(const TextEditor::CompletionItem &item);
    void completionListClosed();

protected:
    bool event(QEvent *e);

    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    friend class CompletionWidget;

    CompletionListView(CompletionSupport *support, ITextEditable *editor, CompletionWidget *completionWidget);

    void setQuickFix(bool quickFix);
    void setCompletionItems(const QList<TextEditor::CompletionItem> &completionitems);
    void keyboardSearch(const QString &search);
    void closeList(const QModelIndex &index);
private slots:
    void maybeShowInfoTip();
private:

    bool m_blockFocusOut;
    bool m_quickFix;
    ITextEditable *m_editor;
    QWidget *m_editorWidget;
    CompletionWidget *m_completionWidget;
    AutoCompletionModel *m_model;
    CompletionSupport *m_support;
    QPointer<CompletionInfoFrame> m_infoFrame;
    QTimer m_infoTimer;
    QChar m_typedChar;
    bool m_explicitlySelected;
};

} // namespace Internal
} // namespace TextEditor

#endif // COMPLETIONWIDGET_H

