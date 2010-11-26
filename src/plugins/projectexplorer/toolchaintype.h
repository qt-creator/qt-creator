/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
    ToolChain_GCC_MAEMO = 9,
    ToolChain_GCCE_GNUPOC = 10,
    ToolChain_RVCT_ARMV5_GNUPOC = 11,
    ToolChain_LAST_VALID = 11,
    ToolChain_OTHER = 200,
    ToolChain_UNKNOWN = 201,
    ToolChain_INVALID = 202
};

} // namespace ProjectExplorer

#endif // TOOLCHAINTYPE_H
