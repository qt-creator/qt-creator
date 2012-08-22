/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "mobilelibraryparameters.h"
#include "qtprojectparameters.h"

#include <QTextStream>
#include <QDir>

namespace Qt4ProjectManager {
namespace Internal {

MobileLibraryParameters::MobileLibraryParameters() :
    type(0), libraryType(TypeNone)
{
}

void MobileLibraryParameters::writeProFile(QTextStream &str) const
{
    if (type&Maemo)
        writeMaemoProFile(str);
}

void MobileLibraryParameters::writeMaemoProFile(QTextStream &str) const
{
    str << "\n"
           "unix:!symbian {\n"
           "    maemo5 {\n"
           "        target.path = /opt/usr/lib\n"
           "    } else {\n"
           "        target.path = /usr/lib\n"
           "    }\n"
           "    INSTALLS += target\n"
           "}\n";
}

} // namespace Internal
} // namespace Qt4ProjectManager
