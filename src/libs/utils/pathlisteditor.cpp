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

#include "pathlisteditor.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QFileDialog>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>

#include <QtCore/QSignalMapper>
#include <QtCore/QMimeData>
#include <QtCore/QSharedPointer>

namespace Core {
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
        text.replace(PathListEditor::separator(), QLatin1Char('\n'));
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
    QPlainTextEdit *edit;
    QSignalMapper *envVarMapper;
    QString fileDialogTitle;
};

PathListEditorPrivate::PathListEditorPrivate()   :
        layout(new QHBoxLayout),
        buttonLayout(new QVBoxLayout),
        edit(new PathListPlainTextEdit),
        envVarMapper(0)
{
    layout->addWidget(edit);
    layout->addLayout(buttonLayout);
    buttonLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
}

PathListEditor::PathListEditor(QWidget *parent) :
        QWidget(parent),
        m_d(new PathListEditorPrivate)
{
    setLayout(m_d->layout);
    addButton(tr("Add..."), this, SLOT(slotAdd()));
    addButton(tr("Delete"), this, SLOT(deletePathAtCursor()));
    addButton(tr("Clear"), this, SLOT(clear()));
}

PathListEditor::~PathListEditor()
{
    delete m_d;
}

QAbstractButton *PathListEditor::addButton(const QString &text, QObject * receiver, const char *slotFunc)
{
    // before Spacer item
    return insertButton(m_d->buttonLayout->count() - 1, text, receiver, slotFunc);
}

QAbstractButton *PathListEditor::insertButton(int index, const QString &text, QObject * receiver, const char *slotFunc)
{
#ifdef Q_OS_MAC
    typedef QPushButton Button;
#else
    typedef QToolButton Button;
#endif
    Button *rc = new Button;
    rc->setText(text);
    connect(rc, SIGNAL(clicked()), receiver, slotFunc);
    m_d->buttonLayout->insertWidget(index, rc);
    return rc;
}

QString PathListEditor::pathListString() const
{
    return pathList().join(separator());
}

QStringList PathListEditor::pathList() const
{
    const QString text = m_d->edit->toPlainText().trimmed();
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
    m_d->edit->setPlainText(l.join(QString(QLatin1Char('\n'))));
}

void PathListEditor::setPathList(const QString &pathString)
{
    if (pathString.isEmpty()) {
        clear();
    } else {
        setPathList(pathString.split(separator(), QString::SkipEmptyParts));
    }
}

void PathListEditor::setPathListFromEnvVariable(const QString &var)
{
    setPathList(qgetenv(var.toLocal8Bit()));
}

QString PathListEditor::fileDialogTitle() const
{
    return m_d->fileDialogTitle;
}

void PathListEditor::setFileDialogTitle(const QString &l)
{
    m_d->fileDialogTitle = l;
}

void PathListEditor::clear()
{
    m_d->edit->clear();
}

void PathListEditor::slotAdd()
{
    const QString dir = QFileDialog::getExistingDirectory(this, m_d->fileDialogTitle);
    if (!dir.isEmpty())
        insertPathAtCursor(dir);
}

QChar PathListEditor::separator()
{
#ifdef Q_OS_WIN
    static const QChar rc(QLatin1Char(';'));
#else
    static const QChar rc(QLatin1Char(':'));
#endif
    return rc;
}

// Add a button "Import from 'Path'"
void PathListEditor::addEnvVariableImportButton(const QString &var)
{
    if (!m_d->envVarMapper) {
        m_d->envVarMapper = new QSignalMapper(this);
        connect(m_d->envVarMapper, SIGNAL(mapped(QString)), this, SLOT(setPathListFromEnvVariable(QString)));
    }

    QAbstractButton *b = addButton(tr("From \"%1\"").arg(var), m_d->envVarMapper, SLOT(map()));
    m_d->envVarMapper->setMapping(b, var);
}

QString PathListEditor::text() const
{
    return m_d->edit->toPlainText();
}

void PathListEditor::setText(const QString &t)
{
    m_d->edit->setPlainText(t);
}

void PathListEditor::insertPathAtCursor(const QString &path)
{
    // If the cursor is at an empty line or at end(),
    // just insert. Else insert line before
    QTextCursor cursor = m_d->edit->textCursor();
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
        m_d->edit->setTextCursor(cursor);
    }
}

void PathListEditor::deletePathAtCursor()
{
    // Delete current line
    QTextCursor cursor = m_d->edit->textCursor();
    if (cursor.block().isValid()) {
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
        m_d->edit->setTextCursor(cursor);
    }
}

} // namespace Utils
} // namespace Core
