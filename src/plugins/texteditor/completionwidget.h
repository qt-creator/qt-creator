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
***************************************************************************/
#ifndef COMPLETIONWIDGET_H
#define COMPLETIONWIDGET_H

#include <QtGui/QListView>
#include <QPointer>

class AutoCompletionModel;

namespace TextEditor {

struct CompletionItem;
class ITextEditable;

namespace Internal {

class CompletionSupport;

/* The completion widget is responsible for showing a list of possible completions.
   It is only used by the CompletionSupport.
 */
class CompletionWidget : public QListView
{
    Q_OBJECT

public:
    CompletionWidget(CompletionSupport *support, ITextEditable *editor);

    void setCompletionItems(const QList<TextEditor::CompletionItem> &completionitems);
    void showCompletions(int startPos);
    void keyboardSearch(const QString &search);
    void closeList(const QModelIndex &index = QModelIndex());

protected:
    bool event(QEvent *e);

signals:
    void itemSelected(const TextEditor::CompletionItem &item);
    void completionListClosed();

private slots:
    void completionActivated(const QModelIndex &index);

private:
    void updateSize();

    QPointer<QFrame> m_popupFrame;
    bool m_blockFocusOut;
    ITextEditable *m_editor;
    QWidget *m_editorWidget;
    AutoCompletionModel *m_model;
    CompletionSupport *m_support;
};

} // namespace Internal
} // namespace TextEditor

#endif // COMPLETIONWIDGET_H

