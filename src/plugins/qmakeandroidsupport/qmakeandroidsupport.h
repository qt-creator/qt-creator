/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include <android/androidqtsupport.h>

namespace QmakeAndroidSupport {
namespace Internal {

class QmakeAndroidSupport : public Android::AndroidQtSupport
{
    Q_OBJECT

public:
    bool canHandle(const ProjectExplorer::Target *target) const override;
    QStringList soLibSearchPath(const ProjectExplorer::Target *target) const override;
    QStringList projectTargetApplications(const ProjectExplorer::Target *target) const override;

    QVariant targetData(Core::Id role, const ProjectExplorer::Target *target) const override;
    bool setTargetData(Core::Id role, const QVariant &value,
                       const ProjectExplorer::Target *target) const override;

    bool parseInProgress(const ProjectExplorer::Target *target) const override;
    bool validParse(const ProjectExplorer::Target *target) const override;
    bool extraLibraryEnabled(const ProjectExplorer::Target *target) const override;
    Utils::FileName projectFilePath(const ProjectExplorer::Target *target) const override;

    void addFiles(const ProjectExplorer::Target *target, const QString &buildKey,
                  const QStringList &addedFiles) const override;
};

} // namespace Internal
} // namespace QmakeAndroidSupport
