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

#include <utils/qtcassert.h>

#include <QDesktopServices>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QUrl>

namespace VcsBase {

VcsOutputLineParser::VcsOutputLineParser() :
    m_regexp(
        "(https?://\\S*)"                             // https://codereview.org/c/1234
        "|(v[0-9]+\\.[0-9]+\\.[0-9]+[\\-A-Za-z0-9]*)" // v0.1.2-beta3
        "|([0-9a-f]{6,}(?:\\.{2,3}[0-9a-f]{6,}"       // 789acf or 123abc..456cde
        "|\\^+|~\\d+)?)")                             // or 789acf^ or 123abc~99
{
}

Utils::OutputLineParser::Result VcsOutputLineParser::handleLine(const QString &text,
                                                                Utils::OutputFormat format)
{
    Q_UNUSED(format);
    QRegularExpressionMatchIterator it = m_regexp.globalMatch(text);
    if (!it.hasNext())
        return Status::NotHandled;
    LinkSpecs linkSpecs;
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const int startPos = match.capturedStart();
        QStringView url = match.capturedView();
        while (url.rbegin()->isPunct())
            url.chop(1);
        linkSpecs << LinkSpec(startPos, url.length(), url.toString());
    }
    return {Status::Done, linkSpecs};
}

bool VcsOutputLineParser::handleVcsLink(const QString &workingDirectory, const QString &href)
{
    using namespace Core;
    QTC_ASSERT(!href.isEmpty(), return false);
    if (href.startsWith("http://") || href.startsWith("https://")) {
        QDesktopServices::openUrl(QUrl(href));
        return true;
    }
    if (IVersionControl *vcs = VcsManager::findVersionControlForDirectory(workingDirectory))
        return vcs->handleLink(workingDirectory, href);
    return false;
}

void VcsOutputLineParser::fillLinkContextMenu(
        QMenu *menu, const QString &workingDirectory, const QString &href)
{
    QTC_ASSERT(!href.isEmpty(), return);
    if (href.startsWith("http://") || href.startsWith("https://")) {
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
