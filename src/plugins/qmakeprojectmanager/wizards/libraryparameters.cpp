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

#include "libraryparameters.h"
#include "librarywizarddialog.h"

#include <utils/codegeneration.h>

#include <QTextStream>
#include <QStringList>

// Contents of the header defining the shared library export.
#define GUARD_VARIABLE "<GUARD>"
#define EXPORT_MACRO_VARIABLE "<EXPORT_MACRO>"
#define LIBRARY_MACRO_VARIABLE "<LIBRARY_MACRO>"

static const char *globalHeaderContentsC =
"#ifndef " GUARD_VARIABLE "\n"
"#define " GUARD_VARIABLE "\n"
"\n"
"#include <QtCore/qglobal.h>\n"
"\n"
"#if defined(" LIBRARY_MACRO_VARIABLE ")\n"
"#  define " EXPORT_MACRO_VARIABLE " Q_DECL_EXPORT\n"
"#else\n"
"#  define " EXPORT_MACRO_VARIABLE " Q_DECL_IMPORT\n"
"#endif\n"
"\n"
"#endif // " GUARD_VARIABLE "\n";

namespace QmakeProjectManager {
namespace Internal {

void LibraryParameters::generateCode(QtProjectParameters:: Type t,
                                     const QString &projectTarget,
                                     const QString &headerName,
                                     const QString &sharedHeader,
                                     const QString &exportMacro,
                                     const QString &pluginJsonFileName,
                                     int indentation,
                                     QString *header,
                                     QString *source) const
{
    QTextStream headerStr(header);

    const QString indent = QString(indentation, QLatin1Char(' '));

    // Do we have namespaces?
    QStringList namespaceList = className.split(QLatin1String("::"));
    if (namespaceList.empty()) // Paranoia!
        return;

    const QString unqualifiedClassName = namespaceList.takeLast();

    // 1) Header
    const QString guard = Utils::headerGuard(headerFileName, namespaceList);
    headerStr << "#ifndef " << guard
        << "\n#define " <<  guard << '\n' << '\n';

    if (!sharedHeader.isEmpty())
        Utils::writeIncludeFileDirective(sharedHeader, false, headerStr);

    // include base class header
    if (!baseClassName.isEmpty()) {
        Utils::writeIncludeFileDirective(baseClassName, true, headerStr);
        headerStr  << '\n';
    }

    const QString namespaceIndent = Utils::writeOpeningNameSpaces(namespaceList, indent, headerStr);

    // Class declaraction
    headerStr << '\n' << namespaceIndent << "class ";
    if (t == QtProjectParameters::SharedLibrary && !exportMacro.isEmpty())
        headerStr << exportMacro << ' ';

    headerStr << unqualifiedClassName;
    if (!baseClassName.isEmpty())
        headerStr << " : public " << baseClassName;
    headerStr << "\n{\n";

    // Is this a QObject (plugin)
    const bool inheritsQObject = t == QtProjectParameters::Qt4Plugin;
    if (inheritsQObject)
        headerStr << namespaceIndent << indent << "Q_OBJECT\n";
    if (t == QtProjectParameters::Qt4Plugin) { // Write Qt 5 plugin meta data.
        const QString qt5InterfaceName = LibraryWizardDialog::pluginInterface(baseClassName);
        if (!qt5InterfaceName.isEmpty()) {
            headerStr << "#if QT_VERSION >= 0x050000\n"
                      << namespaceIndent << indent << "Q_PLUGIN_METADATA(IID \""
                      << qt5InterfaceName << '"';
            if (!pluginJsonFileName.isEmpty())
                headerStr << " FILE \"" << pluginJsonFileName << '"';
            headerStr << ")\n#endif // QT_VERSION >= 0x050000\n";
        }
    }

    headerStr << namespaceIndent << "\npublic:\n";
    if (inheritsQObject)
        headerStr << namespaceIndent << indent << unqualifiedClassName << "(QObject *parent = 0);\n";
    else
        headerStr << namespaceIndent << indent << unqualifiedClassName << "();\n";
    headerStr << namespaceIndent << "};\n\n";
    Utils::writeClosingNameSpaces(namespaceList, indent, headerStr);
    headerStr <<  "#endif // "<<  guard << '\n';
    /// 2) Source
    QTextStream sourceStr(source);

    Utils::writeIncludeFileDirective(headerName, false, sourceStr);
    sourceStr << '\n';

    Utils::writeOpeningNameSpaces(namespaceList, indent, sourceStr);
    // Constructor
    sourceStr << '\n' << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName;
    if (inheritsQObject) {
         sourceStr << "(QObject *parent) :\n"
                   << namespaceIndent << indent << baseClassName << "(parent)\n";
    } else {
        sourceStr << "()\n";
    }
    sourceStr << namespaceIndent << "{\n" << namespaceIndent <<  "}\n";

    Utils::writeClosingNameSpaces(namespaceList, indent, sourceStr);

    if (t == QtProjectParameters::Qt4Plugin) { // Qt 4 plugin export
        sourceStr << "\n#if QT_VERSION < 0x050000\n"
                  << "Q_EXPORT_PLUGIN2(" << projectTarget << ", " << className << ")\n"
                  << "#endif // QT_VERSION < 0x050000\n";
    }
}

QString  LibraryParameters::generateSharedHeader(const QString &globalHeaderFileName,
                                                 const QString &projectTarget,
                                                 const QString &exportMacro)
{
    QString contents = QLatin1String(globalHeaderContentsC);
    contents.replace(QLatin1String(GUARD_VARIABLE), Utils::headerGuard(globalHeaderFileName));
    contents.replace(QLatin1String(EXPORT_MACRO_VARIABLE), exportMacro);
    contents.replace(QLatin1String(LIBRARY_MACRO_VARIABLE), QtProjectParameters::libraryMacro(projectTarget));
    return contents;
}

} // namespace Internal
} // namespace QmakeProjectManager
