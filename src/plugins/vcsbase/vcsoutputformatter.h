// Copyright (C) 2020 Miklos Marton <martonmiklosqdev@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "vcsbase_global.h"

#include <utils/outputformatter.h>

#include <QRegularExpression>

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace VcsBase {

class VCSBASE_EXPORT VcsOutputLineParser : public Utils::OutputLineParser
{
public:
    VcsOutputLineParser();
    void fillLinkContextMenu(QMenu *menu, const Utils::FilePath &workingDirectory, const QString &href);
    Utils::FilePath filePathForLink(const Utils::FilePath &workingDirectory,
                                    const QString &href) const;
    bool handleFileLink(const Utils::FilePath &workingDirectory, const QString &href) const;
    bool handleVcsLink(const Utils::FilePath &workingDirectory, const QString &href);
    static bool shouldOfferFileLink(const QString &href);
    static bool isRevisionLink(const QString &href);
    static QString unquoteGitPath(const QString &token);

private:
    Result handleLine(const QString &text, Utils::OutputFormat format) override;

    const QRegularExpression m_regexp;
};

}
