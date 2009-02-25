/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QTPROJECTPARAMETERS_H
#define QTPROJECTPARAMETERS_H

#include <QtCore/QString>

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
    enum Type { ConsoleApp, GuiApp, StaticLibrary, SharedLibrary, Qt4Plugin };

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
    QString name;
    QString path;
    QString selectedModules;
    QString deselectedModules;
    QString targetDirectory;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QTPROJECTPARAMETERS_H
