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

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <QDesktopServices>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QUrl>

namespace VcsBase {

VcsOutputFormatter::VcsOutputFormatter() :
    m_regexp(
        "(https?://\\S*)"                             // https://codereview.org/c/1234
        "|(v[0-9]+\\.[0-9]+\\.[0-9]+[\\-A-Za-z0-9]*)" // v0.1.2-beta3
        "|([0-9a-f]{6,}(?:\\.{2,3}[0-9a-f]{6,}"       // 789acf or 123abc..456cde
        "|\\^+|~\\d+)?)")                             // or 789acf^ or 123abc~99
{
}

void VcsOutputFormatter::appendMessage(const QString &text, Utils::OutputFormat format)
{
    QRegularExpressionMatchIterator it = m_regexp.globalMatch(text);
    int begin = 0;
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QTextCharFormat normalFormat = charFormat(format);
        OutputFormatter::appendMessage(text.mid(begin, match.capturedStart() - begin), format);
        QTextCursor tc = plainTextEdit()->textCursor();
        QStringView url = match.capturedView();
        begin = match.capturedEnd();
        while (url.rbegin()->isPunct()) {
            url.chop(1);
            --begin;
        }
        tc.movePosition(QTextCursor::End);
        tc.insertText(url.toString(), linkFormat(normalFormat, url.toString()));
        tc.movePosition(QTextCursor::End);
    }
    OutputFormatter::appendMessage(text.mid(begin), format);
}

void VcsOutputFormatter::handleLink(const QString &href)
{
    if (href.startsWith("http://") || href.startsWith("https://"))
        QDesktopServices::openUrl(QUrl(href));
    else if (!href.isEmpty())
        emit referenceClicked(href);
}

void VcsOutputFormatter::fillLinkContextMenu(
        QMenu *menu, const QString &workingDirectory, const QString &href)
{
    if (href.isEmpty() || href.startsWith("http://") || href.startsWith("https://")) {
        QAction *action = menu->addAction(
                    tr("&Open \"%1\"").arg(href),
                    [href] { QDesktopServices::openUrl(QUrl(href)); });
        menu->setDefaultAction(action);
        return;
    }
    if (Core::IVersionControl *vcs = Core::VcsManager::findVersionControlForDirectory(workingDirectory))
        vcs->fillLinkContextMenu(menu, workingDirectory, href);
}

}
