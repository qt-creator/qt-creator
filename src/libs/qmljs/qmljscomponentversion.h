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

#ifndef QMLJSCOMPONENTVERSION_H
#define QMLJSCOMPONENTVERSION_H

#include "qmljs_global.h"

namespace QmlJS {

class QMLJS_EXPORT ComponentVersion
{
    int _major;
    int _minor;

public:
    static const int NoVersion;

    ComponentVersion();
    ComponentVersion(int major, int minor);
    ~ComponentVersion();

    int major() const
    { return _major; }
    int minor() const
    { return _minor; }

    // something in the GNU headers introduces the major() and minor() defines,
    // use these two to disambiguate when necessary
    int majorVersion() const
    { return _major; }
    int minorVersion() const
    { return _minor; }

    bool isValid() const;
};

bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs);

} // namespace QmlJS

#endif // QMLJSCOMPONENTVERSION_H
