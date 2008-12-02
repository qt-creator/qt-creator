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
#ifndef BASETEXTMARK_H
#define BASETEXTMARK_H

#include "itexteditor.h"

namespace TextEditor {

class ITextMarkable;

namespace Internal {
class InternalMark;
}

class TEXTEDITOR_EXPORT BaseTextMark : public QObject
{
    friend class Internal::InternalMark;
    Q_OBJECT
public:
    BaseTextMark();
    BaseTextMark(const QString &filename, int line);
    ~BaseTextMark();

    // return your icon here
    virtual QIcon icon() const = 0;

    // called if the linenumber changes
    virtual void updateLineNumber(int lineNumber) = 0;

    // called whenever the text of the block for the marker changed
    virtual void updateBlock(const QTextBlock &block) = 0;

    // called if the block containing this mark has been removed
    // if this also removes your mark call this->deleteLater();
    virtual void removedFromEditor() = 0;
    // call this if the icon has changed.
    void updateMarker();
    // access to internal data
    QString fileName() const { return m_fileName; }
    int lineNumber() const { return m_line; }

    void moveMark(const QString &filename, int line);
private slots:
    void editorOpened(Core::IEditor *editor);
    void init();
private:
    void childRemovedFromEditor(Internal::InternalMark *mark);
    void documentClosingFor(Internal::InternalMark *mark);

    ITextMarkable *m_markableInterface;
    Internal::InternalMark *m_internalMark;

    QString m_fileName;
    int m_line;
    bool m_init;
};

namespace Internal {

class InternalMark : public ITextMark
{
public:
    InternalMark(BaseTextMark *parent)
        : m_parent(parent)
    {
    }

    ~InternalMark()
    {
    }

    virtual QIcon icon() const
    {
        return m_parent->icon();
    }

    virtual void updateLineNumber(int lineNumber)
    {
        return m_parent->updateLineNumber(lineNumber);
    }

    virtual void updateBlock(const QTextBlock &block)
    {
        return m_parent->updateBlock(block);
    }

    virtual void removedFromEditor()
    {
        m_parent->childRemovedFromEditor(this);
    }

    virtual void documentClosing()
    {
        m_parent->documentClosingFor(this);
    }
private:
    BaseTextMark *m_parent;
};
}
}
#endif // BASETEXTMARK_H
