// Copyright (C) 2020 Miklos Marton <martonmiklosqdev@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsoutputformatter.h"

#include "vcsbasetr.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/vcsmanager.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDesktopServices>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QUrl>

using namespace Utils;

namespace VcsBase {

VcsOutputLineParser::VcsOutputLineParser() :
    m_regexp(
        R"((https?://\S*))"                                           // https://codereview.org/c/1234
        R"(|(?<=[\s"])(v[0-9]+\.[0-9]+\.[0-9]+[\-A-Za-z0-9]*))"       // v0.1.2-beta3
        R"(|(?<=[\s"])(?<!mode )([0-9a-f]{6,}(?:\.{2,3}[0-9a-f]{6,})" // 789acf or 123abc..456cde
        R"(|\^+|~\d+)?)(?=[\s"]))"                                    // or 789acf^ or 123abc~99
        R"(|(?<=\b[ab]/)\S+)")                                        // a/path/to/file.cpp
{
}

OutputLineParser::Result VcsOutputLineParser::handleLine(const QString &text, OutputFormat format)
{
    Q_UNUSED(format)
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

bool VcsOutputLineParser::handleVcsLink(const FilePath &workingDirectory, const QString &href)
{
    using namespace Core;
    QTC_ASSERT(!href.isEmpty(), return false);
    if (href.startsWith("http://") || href.startsWith("https://")) {
        QDesktopServices::openUrl(QUrl(href));
        return true;
    }

    // Do not let a revision that happens to have the same name as a file in
    // the repository root be opened as a file.
    if (isRevisionLink(href)) {
        if (IVersionControl *vcs = VcsManager::findVersionControlForDirectory(workingDirectory))
            return vcs->handleLink(workingDirectory, href);
        return false;
    }

    if (handleFileLink(workingDirectory, href))
        return true;
    if (IVersionControl *vcs = VcsManager::findVersionControlForDirectory(workingDirectory)) {
        return vcs->handleLink(workingDirectory, href);
    }
    return false;
}

bool VcsOutputLineParser::isRevisionLink(const QString &href)
{
    static const QRegularExpression revisionPattern(
        R"(^((v\d+\.\d+\.\d+[-A-Za-z0-9]*)|([0-9a-f]{6,}(?:\.{2,3}[0-9a-f]{6,}|\^+|~\d+)?))$)");
    return revisionPattern.match(href).hasMatch();
}

bool VcsOutputLineParser::shouldOfferFileLink(const QString &href)
{
    return !isRevisionLink(href);
}

QString VcsOutputLineParser::unquoteGitPath(const QString &token)
{
    if (token.size() < 2 || !token.startsWith(QLatin1Char('"'))
        || !token.endsWith(QLatin1Char('"'))) {
        return token;
    }

    const QByteArray quoted = token.mid(1, token.size() - 2).toUtf8();
    QByteArray unquoted;
    for (qsizetype i = 0; i < quoted.size(); ++i) {
        if (quoted.at(i) != '\\' || i + 1 >= quoted.size()) {
            unquoted.append(quoted.at(i));
            continue;
        }

        const char next = quoted.at(++i);
        if (next >= '0' && next <= '7') {
            int value = next - '0';
            for (int count = 1; count < 3 && i + 1 < quoted.size(); ++count) {
                const char digit = quoted.at(i + 1);
                if (digit < '0' || digit > '7')
                    break;
                value = value * 8 + digit - '0';
                ++i;
            }
            unquoted.append(char(value));
        } else {
            unquoted.append(next);
        }
    }
    return QString::fromUtf8(unquoted);
}

static FilePath resolveFileLinkPath(const FilePath &workingDirectory, const QString &href)
{
    const FilePath path = FilePath::fromString(href);
    if (path.isAbsolutePath())
        return workingDirectory.withNewPath(path.path()).cleanPath();
    return workingDirectory.pathAppended(path.path()).cleanPath();
}

static bool isSameOrChildOf(const FilePath &path, const FilePath &parent)
{
    return path == parent || path.isChildOf(parent);
}

VcsOutputLineParser::FileLink VcsOutputLineParser::filePathForLink(
    const FilePath &workingDirectory, const QString &href) const
{
    using namespace Core;

    FileLink result;
    result.filePath = resolveFileLinkPath(workingDirectory, href);
    result.versionControl
        = VcsManager::findVersionControlForDirectory(result.filePath, &result.topLevel);

    // If the target repository contains the working directory, finding the
    // target's version control also validates the working directory.
    if (result.versionControl && isSameOrChildOf(workingDirectory, result.topLevel))
        return result;

    if (!VcsManager::findVersionControlForDirectory(workingDirectory))
        return {}; // The working directory is not under version control.

    // The working directory is version-controlled, while the target file
    // belongs to another repository or is outside version control.
    return result;
}

void VcsOutputLineParser::fillFileLinkContextMenu(QMenu *menu,
                                                  const FilePath &workingDirectory,
                                                  const QString &href) const
{
    if (!shouldOfferFileLink(href))
        return;

    const FileLink fileLink = filePathForLink(workingDirectory, href);
    const FilePath file = fileLink.filePath;
    if (!file.isFile())
        return;

    menu->addSeparator();
    menu->addAction(Tr::tr("Open \"%1\"").arg(file.nativePath()),
                    [file] { Core::EditorManager::openEditor(file.absoluteFilePath()); });

    if (!fileLink.versionControl)
        return;

    const FilePath relativePath = file.relativeChildPath(fileLink.topLevel);
    menu->addSeparator();
    fileLink.versionControl->fillDefaultFileActionMenu(
        menu, fileLink.versionControl, fileLink.topLevel, relativePath);
    fileLink.versionControl->vcsFillFileActionMenu(
        menu, fileLink.topLevel, relativePath, Core::VcsManager::fileState(file));
}

bool VcsOutputLineParser::handleFileLink(const FilePath &workingDirectory,
                                         const QString &href) const
{
    if (!shouldOfferFileLink(href))
        return false;

    const FilePath file = filePathForLink(workingDirectory, href).filePath;
    if (!file.isFile())
        return false;

    Core::EditorManager::openEditor(file);
    return true;
}

void VcsOutputLineParser::fillLinkContextMenu(
        QMenu *menu, const FilePath &workingDirectory, const QString &href)
{
    QTC_ASSERT(!href.isEmpty(), return);
    if (href.startsWith("http://") || href.startsWith("https://")) {
        QAction *action = menu->addAction(Tr::tr("&Open \"%1\"").arg(href),
                                          [href] { QDesktopServices::openUrl(QUrl(href)); });
        menu->setDefaultAction(action);
        menu->addAction(Tr::tr("&Copy to clipboard: \"%1\"").arg(href),
                        [href] { setClipboardAndSelection(href); });
        return;
    }
    if (Core::IVersionControl *vcs = Core::VcsManager::findVersionControlForDirectory(workingDirectory))
        vcs->fillLinkContextMenu(menu, workingDirectory, href);
}

} // VcsBase
