/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "qtprojectparameters.h"

#include <QtCore/QTextStream>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>

namespace Qt4ProjectManager {
namespace Internal {

// ----------- QtProjectParameters
QtProjectParameters::QtProjectParameters() :
   type(ConsoleApp)
{
 }

QString QtProjectParameters::projectPath() const
{
    QString rc = path;
    if (!rc.isEmpty())
        rc += QDir::separator();
    rc += name;
    return rc;
}

void QtProjectParameters::writeProFile(QTextStream &str) const
{
    if (!selectedModules.isEmpty())
        str << "QT       += " << selectedModules << "\n\n";
    if (!deselectedModules.isEmpty())
        str << "QT       -= " << deselectedModules << "\n\n";
    if (!name.isEmpty())
        str << "TARGET = " <<  name << '\n';
    switch (type) {
    case ConsoleApp:
        // Mac: Command line apps should not be bundles
        str << "CONFIG   += console\nCONFIG   -= app_bundle\n\n";
    case GuiApp:
        str << "TEMPLATE = app\n";
        break;
    case StaticLibrary:
        str << "TEMPLATE = lib\nCONFIG += staticlib\n";
        break;
    case SharedLibrary:
        str << "TEMPLATE = lib\n\nDEFINES += " << libraryMacro(name) << '\n';
        break;
    case Qt4Plugin:
        str << "TEMPLATE = lib\nCONFIG += plugin\n";
        break;
    }

    if (!targetDirectory.isEmpty())
        str << "\nDESTDIR = " << targetDirectory << '\n';
}

void QtProjectParameters::writeProFileHeader(QTextStream &str)
{
    const QChar hash = QLatin1Char ('#');
    const QChar nl = QLatin1Char('\n');
    const QChar blank = QLatin1Char(' ');
    // Format as '#-------\n# <Header> \n#---------'
    QString header = QLatin1String(" Project created by ");
    header += QCoreApplication::applicationName();
    header += blank;
    header += QDateTime::currentDateTime().toString(Qt::ISODate);
    const QString line = QString(header.size(), QLatin1Char('-'));
    str << hash << line << nl << hash << nl << hash << header << nl
        << hash <<nl << hash << line << nl << nl;
}


QString createMacro(const QString &name, const QString &suffix)
{
    QString rc = name.toUpper();
    const int extensionPosition = rc.indexOf(QLatin1Char('.'));
    if (extensionPosition != -1)
        rc.truncate(extensionPosition);
    rc += suffix;
    return rc;
}

QString QtProjectParameters::exportMacro(const QString &projectName)
{
    return createMacro(projectName, QLatin1String("SHARED_EXPORT"));
}

QString QtProjectParameters::libraryMacro(const QString &projectName)
{
     return createMacro(projectName, QLatin1String("_LIBRARY"));
}



}
}
