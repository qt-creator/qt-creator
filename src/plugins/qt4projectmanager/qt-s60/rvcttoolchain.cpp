/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "rvcttoolchain.h"
#include <QtCore/QProcess>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

RVCTToolChain::RVCTToolChain(S60Devices::Device device, ToolChain::ToolChainType type)
    : m_versionUpToDate(false),
    m_deviceId(device.id),
    m_deviceName(device.name),
    m_deviceRoot(device.epocRoot),
    m_type(type)
{
}

ToolChain::ToolChainType RVCTToolChain::type() const
{
    return m_type;
}

void RVCTToolChain::updateVersion()
{
    if (m_versionUpToDate)
        return;
    m_versionUpToDate = true;
    m_major = 0;
    m_minor = 0;
    m_build = 0;
    QProcess armcc;
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    addToEnvironment(env);
    armcc.setEnvironment(env.toStringList());
    armcc.start("armcc", QStringList());
    armcc.closeWriteChannel();
    armcc.waitForFinished();
    QString versionLine = armcc.readAllStandardOutput();
    versionLine += armcc.readAllStandardError();
    QRegExp versionRegExp("RVCT(\\d*)\\.(\\d*).*\\[Build.(\\d*)\\]",
        Qt::CaseInsensitive);
    if (versionRegExp.indexIn(versionLine) != -1) {
        m_major = versionRegExp.cap(1).toInt();
	m_minor = versionRegExp.cap(2).toInt();
	m_build = versionRegExp.cap(3).toInt();
    }
}

QByteArray RVCTToolChain::predefinedMacros()
{
    // see http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0205f/Babbacdb.html
    updateVersion();
    QByteArray ba = QString::fromLatin1(
        "#define __arm__arm__\n"
        "#define __ARMCC_VERSION %1%2%3%4\n"
        "#define __ARRAY_OPERATORS\n"
        "#define _BOOL\n"
        "#define c_plusplus\n"
        "#define __cplusplus\n"
        "#define __CC_ARM\n"
        "#define __EDG__\n"
        "#define __STDC__\n"
        "#define __STDC_VERSION__\n"
        "#define __TARGET_FEATURE_DOUBLEWORD\n"
        "#define __TARGET_FEATURE_DSPMUL\n"
        "#define __TARGET_FEATURE_HALFWORD\n"
        "#define __TARGET_FEATURE_THUMB\n"
        "#define _WCHAR_T\n"
        "#define __SYMBIAN32__\n"
        ).arg(m_major, 1, 10, QLatin1Char('0'))
        .arg(m_minor, 1, 10, QLatin1Char('0'))
        .arg("0")
        .arg(m_build, 3, 10, QLatin1Char('0')).toLatin1();
    return ba;
}

QList<HeaderPath> RVCTToolChain::systemHeaderPaths()
{
    if (m_systemHeaderPaths.isEmpty()) {
        updateVersion();
        ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
        QString rvctInclude = env.value(QString::fromLatin1("RVCT%1%2INC").arg(m_major).arg(m_minor));
        if (!rvctInclude.isEmpty())
            m_systemHeaderPaths.append(HeaderPath(rvctInclude, HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\stdapis\\sys").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
        m_systemHeaderPaths.append(HeaderPath(QString("%1\\epoc32\\include\\variant").arg(m_deviceRoot), HeaderPath::GlobalHeaderPath));
    }
    return m_systemHeaderPaths;
}

void RVCTToolChain::addToEnvironment(ProjectExplorer::Environment &env)
{
    env.prependOrSetPath(QString("%1\\epoc32\\tools").arg(m_deviceRoot)); // e.g. make.exe
    env.prependOrSetPath(QString("%1\\epoc32\\gcc\\bin").arg(m_deviceRoot)); // e.g. gcc.exe
    env.set("EPOCDEVICE", QString("%1:%2").arg(m_deviceId, m_deviceName));
    env.set("EPOCROOT", S60Devices::cleanedRootPath(m_deviceRoot));
}

QString RVCTToolChain::makeCommand() const
{
    return "make";
}

bool RVCTToolChain::equals(ToolChain *other) const
{
    return (other->type() == type()
            && m_deviceId == static_cast<RVCTToolChain *>(other)->m_deviceId
            && m_deviceName == static_cast<RVCTToolChain *>(other)->m_deviceName);
}

