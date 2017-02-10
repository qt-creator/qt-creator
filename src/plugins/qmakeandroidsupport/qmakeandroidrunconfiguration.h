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

#include <android/androidrunconfiguration.h>

#include <utils/fileutils.h>

namespace QmakeProjectManager {
class QmakeProFile;
class QmakeProject;
} // namespace QmakeProjectManager

namespace QmakeAndroidSupport {
namespace Internal {

class QmakeAndroidRunConfiguration : public Android::AndroidRunConfiguration
{
    Q_OBJECT
    friend class QmakeAndroidRunConfigurationFactory;

public:
    QmakeAndroidRunConfiguration(ProjectExplorer::Target *parent, Core::Id id,
                                 const Utils::FileName &path = Utils::FileName());

    Utils::FileName proFilePath() const;

    bool isEnabled() const override;
    QString disabledReason() const override;

    QString buildSystemTarget() const final;

protected:
    QmakeAndroidRunConfiguration(ProjectExplorer::Target *parent, QmakeAndroidRunConfiguration *source);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    QString defaultDisplayName();

private:
    void proFileUpdated(QmakeProjectManager::QmakeProFile *pro, bool success, bool parseInProgress);
    QmakeProjectManager::QmakeProject *qmakeProject() const;
    void init();

    mutable Utils::FileName m_proFilePath;
    bool m_parseSuccess;
    bool m_parseInProgress;
};

} // namespace Internal
} // namespace QmakeAndroidSupport
