/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxabstractqtversion.h"

#include "qnxbaseqtconfigwidget.h"

#include <utils/environment.h>

#include <QDir>

using namespace Qnx;
using namespace Qnx::Internal;

QnxAbstractQtVersion::QnxAbstractQtVersion()
    : QtSupport::BaseQtVersion()
    , m_arch(UnknownArch)
{
}

QnxAbstractQtVersion::QnxAbstractQtVersion(QnxArchitecture arch, const Utils::FileName &path, bool isAutoDetected, const QString &autoDetectionSource)
    : QtSupport::BaseQtVersion(path, isAutoDetected, autoDetectionSource)
    , m_arch(arch)
{
    setDefaultSdkPath();
}

QnxArchitecture QnxAbstractQtVersion::architecture() const
{
    return m_arch;
}

QString QnxAbstractQtVersion::archString() const
{
    switch (m_arch) {
    case X86:
        return QLatin1String("x86");
    case ArmLeV7:
        return QLatin1String("ARMle-v7");
    case UnknownArch:
        return QString();
    }

    return QString();
}

QVariantMap QnxAbstractQtVersion::toMap() const
{
    QVariantMap result = BaseQtVersion::toMap();
    result.insert(QLatin1String("SDKPath"), sdkPath());
    result.insert(QLatin1String("Arch"), m_arch);
    return result;
}

void QnxAbstractQtVersion::fromMap(const QVariantMap &map)
{
    BaseQtVersion::fromMap(map);
    setSdkPath(QDir::fromNativeSeparators(map.value(QLatin1String("SDKPath")).toString()));
    m_arch = static_cast<QnxArchitecture>(map.value(QLatin1String("Arch"), UnknownArch).toInt());
}

QList<ProjectExplorer::Abi> QnxAbstractQtVersion::detectQtAbis() const
{
    ensureMkSpecParsed();
    return qtAbisFromLibrary(qtCorePath(versionInfo(), qtVersionString()));
}

void QnxAbstractQtVersion::addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const
{
    QtSupport::BaseQtVersion::addToEnvironment(k, env);

    if (!m_environmentUpToDate)
        updateEnvironment();

    QMultiMap<QString, QString>::const_iterator it;
    QMultiMap<QString, QString>::const_iterator end(m_envMap.constEnd());
    for (it = m_envMap.constBegin(); it != end; ++it) {
        const QString key = it.key();
        const QString value = it.value();

        if (key == QLatin1String("PATH"))
            env.prependOrSetPath(value);
        else if (key == QLatin1String("LD_LIBRARY_PATH"))
            env.prependOrSetLibrarySearchPath(value);
        else
            env.set(key, value);
    }

    env.prependOrSetLibrarySearchPath(versionInfo().value(QLatin1String("QT_INSTALL_LIBS")));
}

QString QnxAbstractQtVersion::sdkPath() const
{
    return m_sdkPath;
}

void QnxAbstractQtVersion::setSdkPath(const QString &sdkPath)
{
    if (m_sdkPath == sdkPath)
        return;

    m_sdkPath = sdkPath;
    m_environmentUpToDate = false;
}

void QnxAbstractQtVersion::updateEnvironment() const
{
    m_envMap = environment();
    m_environmentUpToDate = true;
}

QString QnxAbstractQtVersion::qnxHost() const
{
    if (!m_environmentUpToDate)
        updateEnvironment();

    return m_envMap.value(QLatin1String(Constants::QNX_HOST_KEY));
}

QString QnxAbstractQtVersion::qnxTarget() const
{
    if (!m_environmentUpToDate)
        updateEnvironment();

    return m_envMap.value(QLatin1String(Constants::QNX_TARGET_KEY));
}

QtSupport::QtConfigWidget *QnxAbstractQtVersion::createConfigurationWidget() const
{
    return new QnxBaseQtConfigWidget(const_cast<QnxAbstractQtVersion *>(this));
}

bool QnxAbstractQtVersion::isValid() const
{
    return QtSupport::BaseQtVersion::isValid() && !sdkPath().isEmpty();
}

QString QnxAbstractQtVersion::invalidReason() const
{
    if (sdkPath().isEmpty())
        return tr("No SDK path set");
    return QtSupport::BaseQtVersion::invalidReason();
}

void QnxAbstractQtVersion::setDefaultSdkPath()
{
    QHash<QString, QString> info = versionInfo();
    QString qtHostPrefix;
    if (info.contains(QLatin1String("QT_HOST_PREFIX")))
        qtHostPrefix = info.value(QLatin1String("QT_HOST_PREFIX"));
    else
        return;

    QString envFile;
#if defined Q_OS_WIN
    envFile = qtHostPrefix + QLatin1String("/bbndk-env.bat");
#elif defined Q_OS_UNIX
    envFile = qtHostPrefix + QLatin1String("/bbndk-env.sh");
#endif

    if (QFileInfo(envFile).exists())
        setSdkPath(qtHostPrefix);

}
