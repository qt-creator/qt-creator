/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxconstants.h"
#include "qnxqtversion.h"

#include <qtsupport/baseqtversion.h>

#include <utils/environment.h>

namespace Qnx {
namespace Internal {

class QnxQtVersion : public QtSupport::BaseQtVersion
{
public:
    QnxQtVersion();
    QnxQtVersion(const Utils::FileName &path,
                 bool isAutoDetected = false,
                 const QString &autoDetectionSource = QString());
    QnxQtVersion *clone() const override;

    QString type() const override;

    QString description() const override;

    QSet<Core::Id> availableFeatures() const override;
    QSet<Core::Id> targetDeviceTypes() const override;

    QString qnxHost() const;
    QString qnxTarget() const;

    QString cpuDir() const;

    QVariantMap toMap() const override;
    void fromMap(const QVariantMap &map) override;

    QList<ProjectExplorer::Abi> detectQtAbis() const override;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const override;
    Utils::Environment qmakeRunEnvironment() const override;

    QtSupport::QtConfigWidget *createConfigurationWidget() const override;

    bool isValid() const override;
    QString invalidReason() const override;

    QString sdpPath() const;
    void setSdpPath(const QString &sdpPath);

protected:
   void parseMkSpec(ProFileEvaluator *) const override;

private:
    void updateEnvironment() const;

    QList<Utils::EnvironmentItem> environment() const;

    QString m_sdpPath;

    mutable QString m_cpuDir;
    mutable bool m_environmentUpToDate = false;
    mutable QList<Utils::EnvironmentItem> m_qnxEnv;
};

} // namespace Internal
} // namespace Qnx
