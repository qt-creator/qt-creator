/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef QTPROJECTPARAMETERS_H
#define QTPROJECTPARAMETERS_H

#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextStream;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

// Create a macro name by taking a file name, upper casing it and
// appending a suffix.
QString createMacro(const QString &name, const QString &suffix);

// Base parameters for application project generation with functionality to
// write a .pro-file section.

struct QtProjectParameters {
    enum Type { ConsoleApp, GuiApp, StaticLibrary, SharedLibrary, Qt4Plugin, EmptyProject };
    enum QtVersionSupport { SupportQt4And5, SupportQt4Only, SupportQt5Only };
    enum Flags { WidgetsRequiredFlag = 0x1 };

    QtProjectParameters();
    // Return project path as "path/name"
    QString projectPath() const;
    void writeProFile(QTextStream &) const;
    static void writeProFileHeader(QTextStream &);

    // Shared library: Name of export macro (XXX_EXPORT)
    static QString exportMacro(const QString &projectName);
    // Shared library: name of #define indicating compilation within library
    static QString libraryMacro(const QString &projectName);

    Type type;
    unsigned flags;
    QtVersionSupport qtVersionSupport;
    QString fileName;
    QString target;
    QString path;
    QStringList selectedModules;
    QStringList deselectedModules;
    QString targetDirectory;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTPROJECTPARAMETERS_H
