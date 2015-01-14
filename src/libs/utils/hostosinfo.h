/**************************************************************************
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

#ifndef HOSTOSINFO_H
#define HOSTOSINFO_H

#include "utils_global.h"

#include "osspecificaspects.h"

#include <QString>

#ifdef Q_OS_WIN
#define QTC_HOST_EXE_SUFFIX QTC_WIN_EXE_SUFFIX
#else
#define QTC_HOST_EXE_SUFFIX ""
#endif // Q_OS_WIN

namespace Utils {

class QTCREATOR_UTILS_EXPORT HostOsInfo
{
public:
    static inline OsType hostOs();

    enum HostArchitecture { HostArchitectureX86, HostArchitectureAMD64, HostArchitectureItanium,
                            HostArchitectureArm, HostArchitectureUnknown };
    static HostArchitecture hostArchitecture();

    static bool isWindowsHost() { return hostOs() == OsTypeWindows; }
    static bool isLinuxHost() { return hostOs() == OsTypeLinux; }
    static bool isMacHost() { return hostOs() == OsTypeMac; }
    static inline bool isAnyUnixHost();

    static QString withExecutableSuffix(const QString &executable)
    {
        return hostOsAspects().withExecutableSuffix(executable);
    }

    static Qt::CaseSensitivity fileNameCaseSensitivity()
    {
        return hostOsAspects().fileNameCaseSensitivity();
    }

    static QChar pathListSeparator()
    {
        return hostOsAspects().pathListSeparator();
    }

    static Qt::KeyboardModifier controlModifier()
    {
        return hostOsAspects().controlModifier();
    }

private:
    static OsSpecificAspects hostOsAspects() { return OsSpecificAspects(hostOs()); }
};


OsType HostOsInfo::hostOs()
{
#if defined(Q_OS_WIN)
    return OsTypeWindows;
#elif defined(Q_OS_LINUX)
    return OsTypeLinux;
#elif defined(Q_OS_MAC)
    return OsTypeMac;
#elif defined(Q_OS_UNIX)
    return OsTypeOtherUnix;
#else
    return OsTypeOther;
#endif
}

bool HostOsInfo::isAnyUnixHost()
{
#ifdef Q_OS_UNIX
    return true;
#else
    return false;
#endif
}

} // namespace Utils

#endif // HOSTOSINFO_H
