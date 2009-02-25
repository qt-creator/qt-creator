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

#ifndef TEXTBLOCKITERATOR_H
#define TEXTBLOCKITERATOR_H

#include "texteditor_global.h"

#include <QtGui/QTextBlock>

namespace TextEditor {

/* Iterator for the text blocks of a document. */
class TEXTEDITOR_EXPORT TextBlockIterator {
public:
    TextBlockIterator();
    TextBlockIterator(const QTextBlock &block);

    bool equals(const TextBlockIterator &o) const;

    QString operator*() const;
    TextBlockIterator &operator++();
    TextBlockIterator &operator--();
    TextBlockIterator operator++(int);
    TextBlockIterator operator--(int);

private:
    void read() const;

private:
    const QTextDocument *m_document;
    QTextBlock m_block;
    mutable QString m_text;
    mutable bool m_initialized;
};

inline bool operator==(const TextBlockIterator &i1, const TextBlockIterator &i2) { return i1.equals(i2); }
inline bool operator!=(const TextBlockIterator &i1, const TextBlockIterator &i2) { return !i1.equals(i2); }

} // namespace TextEditor

#endif // TEXTBLOCKITERATOR_H
