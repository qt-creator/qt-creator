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

#ifndef SNIPPETSWINDOW_H
#define SNIPPETSWINDOW_H

#include <QtCore/QDir>
#include <QtGui/QTreeWidget>
#include <QtGui/QSplitter>
#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QDir;
class QLabel;
QT_END_NAMESPACE

namespace TextEditor {
class ITextEditable;
class ITextEditor;
}

namespace Snippets {
namespace Internal {

class SnippetSpec;
class SnippetsTree;
class InputWidget;

class SnippetsWindow : public QSplitter
{
    Q_OBJECT

public:
    SnippetsWindow();
    ~SnippetsWindow();
    const QList<SnippetSpec *> &snippets() const;
    void insertSnippet(TextEditor::ITextEditable *editor, SnippetSpec *snippet);

private slots:
    void updateDescription(QTreeWidgetItem *item);
    void activateSnippet(QTreeWidgetItem *item, int column);
    void setOpenIcon(QTreeWidgetItem *item);
    void setClosedIcon(QTreeWidgetItem *item);

    void showInputWidget(bool canceled, const QString &value);

private:
    void getArguments();
    void replaceAndInsert();
    QString indentOfString(const QString &str, int at = -1);
    void insertIdents(TextEditor::ITextEditable *editor,
        const QString &indent, int fromPos, int toPos);
    QString getCurrentIndent(TextEditor::ITextEditor *editor);

    QList<SnippetSpec *> m_snippets;
    QString createUniqueFileName();
    void writeSnippet(const QMimeData *mData);
    bool initSnippetsDir();
    void initSnippets(const QDir &dir);

    QList<int> m_requiredArgs;
    QStringList m_args;
    SnippetSpec *m_currentSnippet;
    TextEditor::ITextEditable *m_currentEditor;

    QDir m_snippetsDir;

    SnippetsTree *m_snippetsTree;

    QLabel *m_descLabel;

    static const QIcon m_fileIcon;
    static const QIcon m_dirIcon;
    static const QIcon m_dirOpenIcon;
};

class SnippetsTree : public QTreeWidget
{
    Q_OBJECT

public:
    SnippetsTree(QWidget *parent);

protected:
    void dragMoveEvent(QDragMoveEvent * event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
};

} // namespace Internal
} // namespace Snippets

#endif // SNIPPETSWINDOW_H

