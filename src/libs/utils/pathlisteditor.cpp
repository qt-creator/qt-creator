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

#include "pathlisteditor.h"

#include "hostosinfo.h"

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QFileDialog>
#include <QTextBlock>
#include <QMenu>

#include <QSignalMapper>
#include <QMimeData>
#include <QSharedPointer>
#include <QDebug>

/*!
    \class Utils::PathListEditor

    \brief A control that let's the user edit a list of (directory) paths
    using the platform separator (';',':').

    Typically used for
    path lists controlled by environment variables, such as
    PATH. It is based on a QPlainTextEdit as it should
    allow for convenient editing and non-directory type elements like
    \code
    "etc/mydir1:$SPECIAL_SYNTAX:/etc/mydir2".
    \endcode

    When pasting text into it, the platform separator will be replaced
    by new line characters for convenience.
 */

namespace Utils {

// ------------ PathListPlainTextEdit:
// Replaces the platform separator ';',':' by '\n'
// when inserting, allowing for pasting in paths
// from the terminal or such.

class PathListPlainTextEdit : public QPlainTextEdit {
public:
    explicit PathListPlainTextEdit(QWidget *parent = 0);
protected:
    virtual void insertFromMimeData (const QMimeData *source);
};

PathListPlainTextEdit::PathListPlainTextEdit(QWidget *parent) :
    QPlainTextEdit(parent)
{
    // No wrapping, scroll at all events
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setLineWrapMode(QPlainTextEdit::NoWrap);
}

void PathListPlainTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasText()) {
        // replace separator
        QString text = source->text().trimmed();
        text.replace(HostOsInfo::pathListSeparator(), QLatin1Char('\n'));
        QSharedPointer<QMimeData> fixed(new QMimeData);
        fixed->setText(text);
        QPlainTextEdit::insertFromMimeData(fixed.data());
    } else {
        QPlainTextEdit::insertFromMimeData(source);
    }
}

// ------------ PathListEditorPrivate
struct PathListEditorPrivate {
    PathListEditorPrivate();

    QHBoxLayout *layout;
    QVBoxLayout *buttonLayout;
    QToolButton *toolButton;
    QMenu *buttonMenu;
    QPlainTextEdit *edit;
    QSignalMapper *envVarMapper;
    QString fileDialogTitle;
};

PathListEditorPrivate::PathListEditorPrivate()   :
        layout(new QHBoxLayout),
        buttonLayout(new QVBoxLayout),
        toolButton(new QToolButton),
        buttonMenu(new QMenu),
        edit(new PathListPlainTextEdit),
        envVarMapper(0)
{
    layout->setMargin(0);
    layout->addWidget(edit);
    buttonLayout->addWidget(toolButton);
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    layout->addLayout(buttonLayout);
}

PathListEditor::PathListEditor(QWidget *parent) :
        QWidget(parent),
        d(new PathListEditorPrivate)
{
    setLayout(d->layout);
    d->toolButton->setPopupMode(QToolButton::MenuButtonPopup);
    d->toolButton->setText(tr("Insert..."));
    d->toolButton->setMenu(d->buttonMenu);
    connect(d->toolButton, SIGNAL(clicked()), this, SLOT(slotInsert()));

    addAction(tr("Add..."), this, SLOT(slotAdd()));
    addAction(tr("Delete Line"), this, SLOT(deletePathAtCursor()));
    addAction(tr("Clear"), this, SLOT(clear()));
}

PathListEditor::~PathListEditor()
{
    delete d;
}

static inline QAction *createAction(QObject *parent, const QString &text, QObject * receiver, const char *slotFunc)
{
    QAction *rc = new QAction(text, parent);
    QObject::connect(rc, SIGNAL(triggered()), receiver, slotFunc);
    return rc;
}

QAction *PathListEditor::addAction(const QString &text, QObject * receiver, const char *slotFunc)
{
    QAction *rc = createAction(this, text, receiver, slotFunc);
    d->buttonMenu->addAction(rc);
    return rc;
}

QAction *PathListEditor::insertAction(int index /* -1 */, const QString &text, QObject * receiver, const char *slotFunc)
{
    // Find the 'before' action
    QAction *beforeAction = 0;
    if (index >= 0) {
        const QList<QAction*> actions = d->buttonMenu->actions();
        if (index < actions.size())
            beforeAction = actions.at(index);
    }
    QAction *rc = createAction(this, text, receiver, slotFunc);
    if (beforeAction)
        d->buttonMenu->insertAction(beforeAction, rc);
    else
        d->buttonMenu->addAction(rc);
    return rc;
}

int PathListEditor::lastAddActionIndex()
{
    return 0; // Insert/Add
}

QString PathListEditor::pathListString() const
{
    return pathList().join(HostOsInfo::pathListSeparator());
}

QStringList PathListEditor::pathList() const
{
    const QString text = d->edit->toPlainText().trimmed();
    if (text.isEmpty())
        return QStringList();
    // trim each line
    QStringList rc = text.split(QLatin1Char('\n'), QString::SkipEmptyParts);
    const QStringList::iterator end = rc.end();
    for (QStringList::iterator it = rc.begin(); it != end; ++it)
        *it = it->trimmed();
    return rc;
}

void PathListEditor::setPathList(const QStringList &l)
{
    d->edit->setPlainText(l.join(QString(QLatin1Char('\n'))));
}

void PathListEditor::setPathList(const QString &pathString)
{
    if (pathString.isEmpty()) {
        clear();
    } else {
        setPathList(pathString.split(HostOsInfo::pathListSeparator(),
                QString::SkipEmptyParts));
    }
}

void PathListEditor::setPathListFromEnvVariable(const QString &var)
{
    setPathList(QString::fromLocal8Bit(qgetenv(var.toLocal8Bit())));
}

QString PathListEditor::fileDialogTitle() const
{
    return d->fileDialogTitle;
}

void PathListEditor::setFileDialogTitle(const QString &l)
{
    d->fileDialogTitle = l;
}

void PathListEditor::clear()
{
    d->edit->clear();
}

void PathListEditor::slotAdd()
{
    const QString dir = QFileDialog::getExistingDirectory(this, d->fileDialogTitle);
    if (!dir.isEmpty())
        appendPath(QDir::toNativeSeparators(dir));
}

void PathListEditor::slotInsert()
{
    const QString dir = QFileDialog::getExistingDirectory(this, d->fileDialogTitle);
    if (!dir.isEmpty())
        insertPathAtCursor(QDir::toNativeSeparators(dir));
}

// Add a button "Import from 'Path'"
void PathListEditor::addEnvVariableImportAction(const QString &var)
{
    if (!d->envVarMapper) {
        d->envVarMapper = new QSignalMapper(this);
        connect(d->envVarMapper, SIGNAL(mapped(QString)), this, SLOT(setPathListFromEnvVariable(QString)));
    }

    QAction *a = insertAction(lastAddActionIndex() + 1,
                              tr("From \"%1\"").arg(var), d->envVarMapper, SLOT(map()));
    d->envVarMapper->setMapping(a, var);
}

QString PathListEditor::text() const
{
    return d->edit->toPlainText();
}

void PathListEditor::setText(const QString &t)
{
    d->edit->setPlainText(t);
}

void PathListEditor::insertPathAtCursor(const QString &path)
{
    // If the cursor is at an empty line or at end(),
    // just insert. Else insert line before
    QTextCursor cursor = d->edit->textCursor();
    QTextBlock block = cursor.block();
    const bool needNewLine = !block.text().isEmpty();
    if (needNewLine) {
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        cursor.insertBlock();
        cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor);
    }
    cursor.insertText(path);
    if (needNewLine) {
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        d->edit->setTextCursor(cursor);
    }
}

void PathListEditor::appendPath(const QString &path)
{
    QString paths = text().trimmed();
    if (!paths.isEmpty())
        paths += QLatin1Char('\n');
    paths += path;
    setText(paths);
}

void PathListEditor::deletePathAtCursor()
{
    // Delete current line
    QTextCursor cursor = d->edit->textCursor();
    if (cursor.block().isValid()) {
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        // Select down or until end of [last] line
        if (!cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor))
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        d->edit->setTextCursor(cursor);
    }
}

} // namespace Utils
