/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#pragma once

#include <vcsbase/vcsbaseclientsettings.h>

namespace Git {
namespace Internal {

enum CommitType
{
    SimpleCommit,
    AmendCommit,
    FixupCommit
};

// Todo: Add user name and password?
class GitSettings : public VcsBase::VcsBaseClientSettings
{
public:
    GitSettings();

    static const QLatin1String pullRebaseKey;
    static const QLatin1String showTagsKey;
    static const QLatin1String omitAnnotationDateKey;
    static const QLatin1String ignoreSpaceChangesInDiffKey;
    static const QLatin1String ignoreSpaceChangesInBlameKey;
    static const QLatin1String diffPatienceKey;
    static const QLatin1String winSetHomeEnvironmentKey;
    static const QLatin1String showPrettyFormatKey;
    static const QLatin1String gitkOptionsKey;
    static const QLatin1String logDiffKey;
    static const QLatin1String repositoryBrowserCmd;
    static const QLatin1String graphLogKey;
    static const QLatin1String firstParentKey;
    static const QLatin1String lastResetIndexKey;

    Utils::FileName gitExecutable(bool *ok = 0, QString *errorMessage = 0) const;

    GitSettings &operator = (const GitSettings &s);
};

} // namespace Internal
} // namespace Git
