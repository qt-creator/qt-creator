/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TOOLCHAINTYPE_H
#define TOOLCHAINTYPE_H

namespace ProjectExplorer {

enum ToolChainType
{
    ToolChain_GCC = 0,
    ToolChain_LINUX_ICC = 1,
    ToolChain_MinGW = 2,
    ToolChain_MSVC = 3,
    ToolChain_WINCE = 4,
    ToolChain_WINSCW = 5,
    ToolChain_GCCE = 6,
    ToolChain_RVCT2_ARMV5 = 7,
    ToolChain_RVCT2_ARMV6 = 8,
    ToolChain_GCC_MAEMO5 = 9,
    ToolChain_GCCE_GNUPOC = 10,
    ToolChain_RVCT_ARMV5_GNUPOC = 11,
    ToolChain_RVCT4_ARMV5 = 12,
    ToolChain_RVCT4_ARMV6 = 13,
    ToolChain_GCC_HARMATTAN = 14,
    ToolChain_LAST_VALID = 14,
    ToolChain_OTHER = 200,
    ToolChain_UNKNOWN = 201,
    ToolChain_INVALID = 202
};

} // namespace ProjectExplorer

#endif // TOOLCHAINTYPE_H
