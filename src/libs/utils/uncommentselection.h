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

#ifndef UNCOMMENTSELECTION_H
#define UNCOMMENTSELECTION_H

#include "utils_global.h"

#include <QString>

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

} // namespace Utils

#endif // UNCOMMENTSELECTION_H
