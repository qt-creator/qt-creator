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

#ifndef QDBIMPORTS_H
#define QDBIMPORTS_H

#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtGui/QIcon>
#include <QtGui/QPlainTextEdit>

namespace TextEditor {

class BaseTextMark : public QObject
{
//    Q_OBJECT
public:
    BaseTextMark(const QString &/*filename*/, int /*line*/) {}
    ~BaseTextMark() {}

    // return your icon here
    virtual QIcon icon() const { return QIcon(); }

    // called if the linenumber changes
    virtual void updateLineNumber(int /*lineNumber*/) {}

    // called whenever the text of the block for the marker changed
    virtual void updateBlock(const QTextBlock &/*block*/) {}

    // called if the block containing this mark has been removed
    // if this also removes your mark call this->deleteLater();
    virtual void removedFromEditor() {}
    // call this if the icon has changed.
    void updateMarker() {}

public:
    QString m_fileName;
    int m_line;
};

} // namespace TextEditor



#ifdef USE_BASETEXTEDITOR

#define BASETEXTMARK_H
#include "../../plugins/texteditor/basetexteditor.h"


class Editable : public TextEditor::BaseTextEditorEditable
{
    Q_OBJECT

public:
    Editable(TextEditor::BaseTextEditor *editor)
      : TextEditor::BaseTextEditorEditable(editor)
    {}

    // Core::IContext
    QList<int> context() const { return m_context; }

    // Core::IEditor
    const char* kind() const { return "MYEDITOR"; }
    bool duplicateSupported() const { return false; }
    Core::IEditor* duplicate(QWidget*) { return 0; }

    QList<int> m_context;
};


class TextViewer : public TextEditor::BaseTextEditor
{
    Q_OBJECT

public:
    TextViewer(QWidget *parent)
      : TextEditor::BaseTextEditor(parent)
    {
        setCodeFoldingVisible(true);
        setMarksVisible(true);
        setHighlightCurrentLine(true);
    }

    TextEditor::BaseTextEditorEditable* createEditableInterface()
    {
        return new Editable(this);
    }

    QString fileName() { return file()->fileName(); }

    int currentLine() const { return textCursor().blockNumber() + 1; }
};


#else // defined(USE_BASETEXTEDITOR)


class TextViewer : public QPlainTextEdit
{
    Q_OBJECT

public:
    TextViewer(QWidget *parent) : QPlainTextEdit(parent) {}
    QString fileName() const { return m_fileName; }
    int currentLine() const { return textCursor().blockNumber() + 1; }

    bool open(const QString &fileName)
    {
        m_fileName = fileName;
        QFile file(fileName);
        bool result = file.open(QIODevice::ReadOnly);
        QString contents = file.readAll();
        setPlainText(contents);
        return result;
    }

public:    
    QString m_fileName;
};

#endif // defined(USE_BASETEXTEDITOR)

#endif // QDBIMPORTS
