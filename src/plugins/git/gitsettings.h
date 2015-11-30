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

#ifndef GITSETTINGS_H
#define GITSETTINGS_H

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
    static const QLatin1String lastResetIndexKey;

    Utils::FileName gitExecutable(bool *ok = 0, QString *errorMessage = 0) const;

    GitSettings &operator = (const GitSettings &s);
};

} // namespace Internal
} // namespace Git

#endif // GITSETTINGS_H
