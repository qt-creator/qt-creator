/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef UNCOMMENTSELECTION_H
#define UNCOMMENTSELECTION_H

#include "utils_global.h"

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT CommentDefinition
{
public:
    CommentDefinition();

    CommentDefinition &setAfterWhiteSpaces(const bool);
    CommentDefinition &setSingleLine(const QString &singleLine);
    CommentDefinition &setMultiLineStart(const QString &multiLineStart);
    CommentDefinition &setMultiLineEnd(const QString &multiLineEnd);

    bool isAfterWhiteSpaces() const;
    const QString &singleLine() const;
    const QString &multiLineStart() const;
    const QString &multiLineEnd() const;

    bool hasSingleLineStyle() const;
    bool hasMultiLineStyle() const;

    void clearCommentStyles();

private:
    bool m_afterWhiteSpaces;
    QString m_singleLine;
    QString m_multiLineStart;
    QString m_multiLineEnd;
};

QTCREATOR_UTILS_EXPORT
void unCommentSelection(QPlainTextEdit *edit,
                        const CommentDefinition &definiton = CommentDefinition());

} // end of namespace Utils

#endif // UNCOMMENTSELECTION_H
