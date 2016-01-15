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

#include "embeddedlinuxqtversionfactory.h"

#include "embeddedlinuxqtversion.h"
#include "remotelinux_constants.h"

#include <QFileInfo>

namespace RemoteLinux {
namespace Internal {

EmbeddedLinuxQtVersionFactory::EmbeddedLinuxQtVersionFactory(QObject *parent) : QtSupport::QtVersionFactory(parent)
{ }

EmbeddedLinuxQtVersionFactory::~EmbeddedLinuxQtVersionFactory()
{ }

bool EmbeddedLinuxQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(RemoteLinux::Constants::EMBEDDED_LINUX_QT);
}

QtSupport::BaseQtVersion *EmbeddedLinuxQtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    if (!canRestore(type))
        return 0;
    EmbeddedLinuxQtVersion *v = new EmbeddedLinuxQtVersion;
    v->fromMap(data);
    return v;
}

int EmbeddedLinuxQtVersionFactory::priority() const
{
    return 10;
}

QtSupport::BaseQtVersion *EmbeddedLinuxQtVersionFactory::create(const Utils::FileName &qmakePath,
                                                                ProFileEvaluator *evaluator,
                                                                bool isAutoDetected,
                                                                const QString &autoDetectionSource)
{
    Q_UNUSED(evaluator);

    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    EmbeddedLinuxQtVersion *version = new EmbeddedLinuxQtVersion(qmakePath, isAutoDetected, autoDetectionSource);

    QList<ProjectExplorer::Abi> abis = version->qtAbis();
    // Note: This fails for e.g. intel/meego cross builds on x86 linux machines.
    if (abis.count() == 1
            && abis.at(0).os() == ProjectExplorer::Abi::LinuxOS
            && !ProjectExplorer::Abi::hostAbi().isCompatibleWith(abis.at(0)))
        return version;

    delete version;
    return 0;
}

} // namespace Internal
} // namespace RemoteLinux
