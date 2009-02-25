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

#include "snippetswindow.h"
#include "snippetspec.h"
#include "inputwidget.h"
#include "snippetsplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/itexteditable.h>
#include <texteditor/itexteditor.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtCore/QMimeData>
#include <QtGui/QHeaderView>

using namespace Snippets::Internal;

const QIcon SnippetsWindow::m_fileIcon = QIcon(":/snippets/images/file.png");
const QIcon SnippetsWindow::m_dirIcon = QIcon(":/snippets/images/dir.png");
const QIcon SnippetsWindow::m_dirOpenIcon = QIcon(":/snippets/images/diropen.png");

Q_DECLARE_METATYPE(Snippets::Internal::SnippetSpec *)

SnippetsWindow::SnippetsWindow()
{
    setWindowTitle(tr("Snippets"));
    setWindowIcon(QIcon(":/snippets/images/snippets.png"));
    setOrientation(Qt::Vertical);

    m_snippetsTree = new SnippetsTree(this);
    addWidget(m_snippetsTree);

    m_descLabel = new QLabel(this);
    m_descLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    m_descLabel->setFrameShape(QFrame::Panel);
    m_descLabel->setFrameShadow(QFrame::Raised);
    m_descLabel->setWordWrap(true);
    addWidget(m_descLabel);

    m_snippetsDir = QDir::home();
    if (!initSnippetsDir())
        setDisabled(true);
    else {
        QDir defaultDir(Core::ICore::instance()->resourcePath() + QLatin1String("/snippets"));
        if (defaultDir.exists())
            initSnippets(defaultDir);
        initSnippets(m_snippetsDir);
    }

    connect(m_snippetsTree, SIGNAL(itemCollapsed(QTreeWidgetItem *)),
        this, SLOT(setClosedIcon(QTreeWidgetItem *)));

    connect(m_snippetsTree, SIGNAL(itemExpanded(QTreeWidgetItem *)),
        this, SLOT(setOpenIcon(QTreeWidgetItem *)));

    connect(m_snippetsTree, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
        this, SLOT(activateSnippet(QTreeWidgetItem *, int)));

    connect(m_snippetsTree, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
        this, SLOT(updateDescription(QTreeWidgetItem *)));
}

SnippetsWindow::~SnippetsWindow()
{
    qDeleteAll(m_snippets);
}


void SnippetsWindow::activateSnippet(QTreeWidgetItem *item, int column)
{
    if (!item->parent())
        return;

    TextEditor::ITextEditable *editor = 0;
    if (Core::ICore::instance()->editorManager()->currentEditor())
        editor = qobject_cast<TextEditor::ITextEditable *>(
                Core::ICore::instance()->editorManager()->currentEditor());
    if (editor) {
        SnippetSpec* spec = qVariantValue<SnippetSpec*>(item->data(0, Qt::UserRole));
        insertSnippet(editor, spec);
    }

    Q_UNUSED(column);
}

const QList<SnippetSpec *> &SnippetsWindow::snippets() const
{
    return m_snippets;
}

void SnippetsWindow::initSnippets(const QDir &dir)
{
    QString name;
    QString category;

    QMap<QString, QTreeWidgetItem *> categories;
    for (int i = 0; i < m_snippetsTree->topLevelItemCount(); ++i) {
        categories.insert(m_snippetsTree->topLevelItem(i)->text(0),
            m_snippetsTree->topLevelItem(i));
    }

    foreach (const QString &snippet, dir.entryList(QStringList("*.snp"))) {
        SnippetSpec *spec = new SnippetSpec();
        if (spec->load(dir.filePath(snippet))) {
            if (!categories.contains(spec->category())) {
                QTreeWidgetItem *citem = new QTreeWidgetItem(m_snippetsTree);
                citem->setText(0, spec->category());
                citem->setIcon(0, m_dirIcon);
                categories.insert(spec->category(), citem);
            }

            QTreeWidgetItem *item = new QTreeWidgetItem(
                categories.value(spec->category()));
            item->setText(0, spec->name());
            item->setIcon(0, m_fileIcon);
            QVariant v;
            qVariantSetValue<SnippetSpec *>(v, spec);
            item->setData(0, Qt::UserRole, v);

            m_snippets.append(spec);
        }
    }
}

QString SnippetsWindow::createUniqueFileName()
{
    int fileNumber = 0;
    QString baseName = "snippet";
    while (m_snippetsDir.exists(baseName + QString::number(fileNumber) + ".snp")) {
        ++fileNumber;
    }
    return baseName + QString::number(fileNumber) + ".snp";
}

void SnippetsWindow::writeSnippet(const QMimeData *)
{
}

bool SnippetsWindow::initSnippetsDir()
{
    if (!m_snippetsDir.exists(".qworkbench"))
        m_snippetsDir.mkdir(".qworkbench");
    if (!m_snippetsDir.cd(".qworkbench"))
        return false;

    if (!m_snippetsDir.exists("snippets"))
        m_snippetsDir.mkdir("snippets");
    return m_snippetsDir.cd("snippets");
}

void SnippetsWindow::getArguments()
{
    QString contents = m_currentSnippet->contents();
    int index = 0;
    bool pc = false;
    QString nrstr;

    QSet<int> requiredArgs;
    m_requiredArgs.clear();
    m_args.clear();

    while (index < contents.length()) {
        QChar c = contents.at(index);
        if (c == QLatin1Char('%')) {
            pc = !pc;
        } else if (pc) {
            if (c.isNumber()) {
                nrstr += c;
            } else {
                pc = false;
            }
        }

        if (!pc && !nrstr.isEmpty()) {
            requiredArgs << nrstr.toInt();
            nrstr.clear();
        }

        ++index;
    }

    m_requiredArgs = requiredArgs.toList();
    m_requiredArgs.prepend(-1);

    showInputWidget(false, QString());
}

void SnippetsWindow::showInputWidget(bool canceled, const QString &value)
{
    if (canceled)
        return;

    TextEditor::ITextEditor *te = 0;
    if (Core::ICore::instance()->editorManager()->currentEditor())
        te = qobject_cast<TextEditor::ITextEditor*>(
                Core::ICore::instance()->editorManager()->currentEditor());

    int arg = m_requiredArgs.takeFirst();
    if (arg != -1)
        m_args << value;

    if (!te || m_requiredArgs.isEmpty()) {
        qDebug("replaceAndInsert");
        replaceAndInsert();
    } else {
        QString desc = m_currentSnippet->argumentDescription(m_requiredArgs.first());
        QString def = m_currentSnippet->argumentDefault(m_requiredArgs.first());
        foreach (const QString &arg, m_args) {
            desc = desc.arg(arg);
            def = def.arg(arg);
        }

        InputWidget *iw = new InputWidget(desc, def);
        connect(iw, SIGNAL(finished(bool, const QString &)),
            this, SLOT(showInputWidget(bool, const QString &)));
        iw->showInputWidget(te->cursorRect().bottomRight());
    }
}

void SnippetsWindow::replaceAndInsert()
{
    QString result;
    QString keyWord;
    int setAnchor = -1;
    int setCursor = -1;
    int selLength = 0;

    //clean up selection
    int startPos = m_currentEditor->position(TextEditor::ITextEditable::Anchor);
    int endPos = m_currentEditor->position();

    if (startPos < 0) {
        startPos = endPos;
    } else {
        if (startPos > endPos) {
            int tmp = startPos;
            startPos = endPos;
            endPos = tmp;
        }
        selLength = endPos - startPos;
    }

    //parse the contents
    m_currentEditor->setCurPos(startPos);
    QString editorIndent = getCurrentIndent(m_currentEditor);
    QString content = m_currentSnippet->contents();
    foreach (const QString &arg, m_args) {
        content = content.arg(arg);
    }

    int startOfKey = -1;
    for (int i = 0; i<content.length(); ++i) {
        //handle windows,mac and linux new lines...
        if (content.at(i) == QLatin1Char('\n')) {
            if ((i <= 0) || content.at(i-1) != QLatin1Char('\r'))
                result += QLatin1Char('\n') + editorIndent;
            continue;
        } else if (content.at(i) == QLatin1Char('\r')) {
            result += QLatin1Char('\n') + editorIndent;
            continue;
        }

        if (content.at(i) == QChar('$')) {
            if (startOfKey != -1) {
                m_currentEditor->insert(result);
                if (keyWord == QLatin1String("selection")) {
                    const QString &indent = indentOfString(content, i);
                    int selStartPos = m_currentEditor->position();
                    m_currentEditor->setCurPos(selStartPos + selLength);
                    insertIdents(m_currentEditor, indent, selStartPos, m_currentEditor->position());
                } else if (keyWord == QLatin1String("anchor")) {
                    setAnchor = m_currentEditor->position();
                } else if (keyWord == QLatin1String("cursor")) {
                    setCursor = m_currentEditor->position();
                }
                result.clear();
                keyWord.clear();
                startOfKey = -1;
            } else {
                startOfKey = i;
            }
        } else {
            if (startOfKey != -1)
                keyWord += content.at(i).toLower();
            else
                result += content.at(i);
        }
    }

    m_currentEditor->insert(result);

    if (setAnchor != -1) {
        m_currentEditor->setCurPos(setAnchor);
        m_currentEditor->select(setCursor);
    } else if (setCursor != -1) {
        m_currentEditor->setCurPos(setCursor);
    }
}

void SnippetsWindow::insertSnippet(TextEditor::ITextEditable *editor, SnippetSpec *snippet)
{
    m_currentEditor = editor;
    m_currentSnippet = snippet;
    getArguments();
}

QString SnippetsWindow::getCurrentIndent(TextEditor::ITextEditor *editor)
{
    const int startPos = editor->position(TextEditor::ITextEditor::StartOfLine);
    const int endPos = editor->position(TextEditor::ITextEditor::EndOfLine);
    if (startPos < endPos)
        return indentOfString(editor->textAt(startPos, endPos - startPos));
    return QString();
}

void SnippetsWindow::insertIdents(TextEditor::ITextEditable *editor,
                                  const QString &indent, int fromPos, int toPos)
{
    int offset = 0;
    const int startPos = editor->position();
    editor->setCurPos(toPos);
    int currentLinePos = editor->position(TextEditor::ITextEditor::StartOfLine);
    while (currentLinePos > fromPos) {
        editor->setCurPos(currentLinePos);
        editor->insert(indent);
        offset += indent.length();
        editor->setCurPos(currentLinePos-1);
        currentLinePos = editor->position(TextEditor::ITextEditor::StartOfLine);
    }
    editor->setCurPos(startPos + offset);
}

QString SnippetsWindow::indentOfString(const QString &str, int at)
{
    QString result;
    int startAt = at;
    if (startAt < 0)
        startAt = str.length() - 1;

    // find start position
    while (startAt >= 0 && str.at(startAt) != QChar('\n')
        && str.at(startAt) != QChar('\r')) --startAt;

    for (int i = (startAt + 1); i < str.length(); ++i) {
        if (str.at(i) == QChar(' ') || str.at(i) == QChar('\t'))
            result += str.at(i);
        else
            break;
    }

    return result;
}

void SnippetsWindow::setOpenIcon(QTreeWidgetItem *item)
{
    item->setIcon(0, m_dirOpenIcon);
}

void SnippetsWindow::setClosedIcon(QTreeWidgetItem *item)
{
    item->setIcon(0, m_dirIcon);
}

void SnippetsWindow::updateDescription(QTreeWidgetItem *item)
{
    const SnippetSpec* spec = qVariantValue<SnippetSpec*>(item->data(0, Qt::UserRole));
    if (spec) {
        m_descLabel->setText(QLatin1String("<b>") + spec->name() + QLatin1String("</b><br>")
            + spec->description());
    } else {
        m_descLabel->setText(QLatin1String("<b>") + item->text(0) + QLatin1String("</b><br>"));
    }
}

SnippetsTree::SnippetsTree(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(1);
    header()->setVisible(false);
    setAlternatingRowColors(true);
    setAcceptDrops(true);
}

void SnippetsTree::dropEvent(QDropEvent *)
{
    //writeSnippet(event->mimeData());
}

void SnippetsTree::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void SnippetsTree::dragMoveEvent(QDragMoveEvent *)
{
}
