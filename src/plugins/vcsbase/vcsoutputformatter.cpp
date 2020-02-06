/****************************************************************************
**
** Copyright (C) 2020 Miklos Marton <martonmiklosqdev@gmail.com>
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#include "vcsoutputformatter.h"

#include <QDesktopServices>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QUrl>

namespace VcsBase {

VcsOutputFormatter::VcsOutputFormatter() : m_urlRegexp("https?://\\S*") {}

void VcsOutputFormatter::appendMessage(const QString &text, Utils::OutputFormat format)
{
    QString out = text;
    const QRegularExpressionMatch match = m_urlRegexp.match(text);
    if (match.hasMatch()) {
        const QTextCharFormat normalFormat = charFormat(format);
        OutputFormatter::appendMessage(text.left(match.capturedStart()), format);
        QTextCursor tc = plainTextEdit()->textCursor();
        QStringRef url = match.capturedRef();
        int end = match.capturedEnd();
        while (url.rbegin()->isPunct()) {
            url.chop(1);
            --end;
        }
        tc.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        tc.insertText(url.toString(), linkFormat(normalFormat, url.toString()));
        tc.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        OutputFormatter::appendMessage(text.mid(end), format);
    } else {
        OutputFormatter::appendMessage(text, format);
    }
}

void VcsOutputFormatter::handleLink(const QString &href)
{
    QDesktopServices::openUrl(QUrl(href));
}

}
