/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ASSISTINTERFACE_H
#define ASSISTINTERFACE_H

#include <QTextCursor>

#include <QTextDocument>

#include "../assistenums.h"

namespace TextEditor {

class AssistInterface
{
public:
    AssistInterface(const QByteArray &text,
                    int position)
        : textDocument_(QString::fromUtf8(text)),
          position_(position)
    {}

    AssistInterface(QTextDocument *textDocument,
                    int position,
                    const QString &fileName,
                    AssistReason reason)
        : textDocument_(textDocument),
          fileName_(fileName),
          position_(position),
          reason_(reason)
    {}

    QTextDocument *textDocument() const;
    virtual int position() const;
    virtual QChar characterAt(int position) const;
    virtual QString textAt(int position, int length) const;
    virtual QString fileName() const;
    virtual AssistReason reason() const;


private:
    mutable QTextDocument textDocument_;
    QString fileName_;
    int position_;
    AssistReason reason_ = IdleEditor;
};

inline QTextDocument *AssistInterface::textDocument() const
{
    return &textDocument_;
}

inline int AssistInterface::position() const
{
    return position_;
}

inline QChar AssistInterface::characterAt(int position) const
{
    return textDocument_.characterAt(position);
}

inline QString AssistInterface::textAt(int position, int length) const
{
    QTextCursor textCursor(&textDocument_);
    if (position < 0)
        position = 0;
    textCursor.movePosition(QTextCursor::End);
    if (position + length > textCursor.position())
        length = textCursor.position() - position;

    textCursor.setPosition(position);
    textCursor.setPosition(position + length, QTextCursor::KeepAnchor);

    // selectedText() returns U+2029 (PARAGRAPH SEPARATOR) instead of newline
    return textCursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));
}

inline AssistReason AssistInterface::reason() const
{
    return reason_;
}

inline QString AssistInterface::fileName() const
{
    return fileName_;
}
}

#endif // ASSISTINTERFACE_H

