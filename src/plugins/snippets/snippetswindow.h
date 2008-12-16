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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

namespace Core {
class ICore;
}

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

    Core::ICore *m_core;
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

