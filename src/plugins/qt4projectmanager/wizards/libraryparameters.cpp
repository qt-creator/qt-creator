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

#include "libraryparameters.h"

#include <utils/codegeneration.h>

#include <QtCore/QTextStream>
#include <QtCore/QStringList>

// Contents of the header defining the shared library export.
#define GUARD_VARIABLE "<GUARD>"
#define EXPORT_MACRO_VARIABLE "<EXPORT_MACRO>"
#define LIBRARY_MACRO_VARIABLE "<LIBRARY_MACRO>"

static const char *globalHeaderContentsC =
"#ifndef "GUARD_VARIABLE"\n"
"#define "GUARD_VARIABLE"\n"
"\n"
"#include <QtCore/qglobal.h>\n"
"\n"
"#if defined("LIBRARY_MACRO_VARIABLE")\n"
"#  define "EXPORT_MACRO_VARIABLE" Q_DECL_EXPORT\n"
"#else\n"
"#  define "EXPORT_MACRO_VARIABLE" Q_DECL_IMPORT\n"
"#endif\n"
"\n"
"#endif // "GUARD_VARIABLE"\n";

namespace Qt4ProjectManager {
namespace Internal {

void LibraryParameters::generateCode(QtProjectParameters:: Type t,
                                     const QString &projectTarget,
                                     const QString &headerName,
                                     const QString &sharedHeader,
                                     const QString &exportMacro,
                                     int indentation,
                                     QString *header,
                                     QString *source) const
{
    QString rc;
    QTextStream headerStr(header);

    const QString indent = QString(indentation, QLatin1Char(' '));

    // 1) Header
    const QString guard = Core::Utils::headerGuard(headerFileName);
    headerStr << "#ifndef " << guard
        << "\n#define " <<  guard << '\n' << '\n';

    if (!sharedHeader.isEmpty())
        Core::Utils::writeIncludeFileDirective(sharedHeader, false, headerStr);

    // include base class header
    if (!baseClassName.isEmpty()) {
        QString include;
        if (!baseClassModule.isEmpty()) {
            include += baseClassModule;
            include += QLatin1Char('/');
        }
        include += baseClassName;
        Core::Utils::writeIncludeFileDirective(include, true, headerStr);
        headerStr  << '\n';
    }

    // Do we have namespaces?
    QStringList namespaceList = className.split(QLatin1String("::"));
    if (namespaceList.empty()) // Paranoia!
        return;

    const QString unqualifiedClassName = namespaceList.back();
    namespaceList.pop_back();

    const QString namespaceIndent = Core::Utils::writeOpeningNameSpaces(namespaceList, indent, headerStr);

    // Class declaraction
    headerStr << '\n' << namespaceIndent << "class ";
    if (t == QtProjectParameters::SharedLibrary && !exportMacro.isEmpty())
        headerStr << exportMacro << ' ';

    headerStr << unqualifiedClassName;
    if (!baseClassName.isEmpty())
        headerStr << " : public " << baseClassName;
    headerStr << " {\n";

    // Is this a QObject (plugin)
    const bool inheritsQObject = t == QtProjectParameters::Qt4Plugin;
    if (inheritsQObject) {
        headerStr << namespaceIndent << indent << "Q_OBJECT\n"
            << namespaceIndent << indent << "Q_DISABLE_COPY(" << unqualifiedClassName << ")\n";
    }
    headerStr << namespaceIndent << "public:\n";
    if (inheritsQObject) {
        headerStr << namespaceIndent << indent << "explicit " << unqualifiedClassName << "(QObject *parent = 0);\n";
    } else {
        headerStr << namespaceIndent << indent << unqualifiedClassName << "();\n";
    }
    headerStr << namespaceIndent << "};\n\n";
    Core::Utils::writeClosingNameSpaces(namespaceList, indent, headerStr);
    headerStr <<  "#endif // "<<  guard << '\n';
    /// 2) Source
    QTextStream sourceStr(source);

    Core::Utils::writeIncludeFileDirective(headerName, false, sourceStr);
    sourceStr << '\n';

    Core::Utils::writeOpeningNameSpaces(namespaceList, indent, sourceStr);
    // Constructor
    sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName;
    if (inheritsQObject) {
         sourceStr << "(QObject *parent) :\n"
                   << namespaceIndent << indent << baseClassName << "(parent)\n";
    } else {
        sourceStr << "()\n";
    }
    sourceStr << namespaceIndent << "{\n" << namespaceIndent <<  "}\n";

    Core::Utils::writeClosingNameSpaces(namespaceList, indent, sourceStr);

    if (t == QtProjectParameters::Qt4Plugin)
        sourceStr << '\n' << "Q_EXPORT_PLUGIN2(" << projectTarget << ", " << className << ")\n";
}

QString  LibraryParameters::generateSharedHeader(const QString &globalHeaderFileName,
                                                 const QString &projectTarget,
                                                 const QString &exportMacro)
{
    QString contents = QLatin1String(globalHeaderContentsC);
    contents.replace(QLatin1String(GUARD_VARIABLE), Core::Utils::headerGuard(globalHeaderFileName));
    contents.replace(QLatin1String(EXPORT_MACRO_VARIABLE), exportMacro);
    contents.replace(QLatin1String(LIBRARY_MACRO_VARIABLE), QtProjectParameters::libraryMacro(projectTarget));
    return contents;
}

} // namespace Internal
} // namespace Qt4ProjectManager
