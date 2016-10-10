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

#include "qtprojectparameters.h"
#include <utils/codegeneration.h>

#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace QmakeProjectManager {
namespace Internal {

// ----------- QtProjectParameters
QtProjectParameters::QtProjectParameters()
  : type(ConsoleApp), flags(0), qtVersionSupport(SupportQt4And5)
{
}

QString QtProjectParameters::projectPath() const
{
    QString rc = path;
    if (!rc.isEmpty())
        rc += QLatin1Char('/');
    rc += fileName;
    return rc;
}

// Write out a QT module line.
static inline void writeQtModulesList(QTextStream &str,
                                      const QStringList &modules,
                                      char op ='+')
{
    if (const int size = modules.size()) {
        str << "QT       " << op << "= ";
        for (int i =0; i < size; ++i) {
            if (i)
                str << ' ';
            str << modules.at(i);
        }
        str << "\n\n";
    }
}

void QtProjectParameters::writeProFile(QTextStream &str) const
{
    QStringList allSelectedModules = selectedModules;
    // Handling of widgets module.
    const bool addWidgetsModule =
        (flags & WidgetsRequiredFlag) && qtVersionSupport != SupportQt4Only
        && !allSelectedModules.contains(QLatin1String("widgets"));

    const bool addConditionalPrintSupport = qtVersionSupport == SupportQt4And5
        && allSelectedModules.removeAll(QLatin1String("printsupport")) > 0;

    if (addWidgetsModule && qtVersionSupport == SupportQt5Only)
        allSelectedModules.append(QLatin1String("widgets"));
    writeQtModulesList(str, allSelectedModules, '+');
    writeQtModulesList(str, deselectedModules, '-');
    if (addWidgetsModule && qtVersionSupport == SupportQt4And5)
        str << "greaterThan(QT_MAJOR_VERSION, 4): QT += widgets\n\n";
    if (addConditionalPrintSupport)
        str << "greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport\n\n";

    const QString &effectiveTarget = target.isEmpty() ? fileName : target;
    if (!effectiveTarget.isEmpty())
        str << "TARGET = " <<  effectiveTarget << '\n';
    switch (type) {
    case ConsoleApp:
        // Mac: Command line apps should not be bundles
        str << "CONFIG   += console\nCONFIG   -= app_bundle\n\n";
        // fallthrough
    case GuiApp:
        str << "TEMPLATE = app\n";
        break;
    case StaticLibrary:
        str << "TEMPLATE = lib\nCONFIG += staticlib\n";
        break;
    case SharedLibrary:
        str << "TEMPLATE = lib\n\nDEFINES += " << libraryMacro(fileName) << '\n';
        break;
    case Qt4Plugin:
        str << "TEMPLATE = lib\nCONFIG += plugin\n";
        break;
    default:
        break;
    }

    if (!targetDirectory.isEmpty())
        str << "\nDESTDIR = " << targetDirectory << '\n';

    if (qtVersionSupport != SupportQt4Only) {
        str << "\n"
               "# The following define makes your compiler emit warnings if you use\n"
               "# any feature of Qt which as been marked as deprecated (the exact warnings\n"
               "# depend on your compiler). Please consult the documentation of the\n"
               "# deprecated API in order to know how to port your code away from it.\n"
               "DEFINES += QT_DEPRECATED_WARNINGS\n\n"
               "# You can also make your code fail to compile if you use deprecated APIs.\n"
               "# In order to do so, uncomment the following line.\n"
               "# You can also select to disable deprecated APIs only up to a certain version of Qt.\n"
               "#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0\n";
    }
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
    return Utils::fileNameToCppIdentifier(rc);
}

QString QtProjectParameters::exportMacro(const QString &projectName)
{
    return createMacro(projectName, QLatin1String("SHARED_EXPORT"));
}

QString QtProjectParameters::libraryMacro(const QString &projectName)
{
     return createMacro(projectName, QLatin1String("_LIBRARY"));
}

} // namespace Internal
} // namespace QmakeProjectManager
