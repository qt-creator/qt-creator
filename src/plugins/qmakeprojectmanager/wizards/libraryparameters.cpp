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
#include <utils/qtcassert.h>

#include <QTextStream>
#include <QStringList>

#include <cstring>

namespace QmakeProjectManager {
namespace Internal {

void LibraryParameters::generateCode(QtProjectParameters:: Type t,
                                     const QString &headerName,
                                     const QString &sharedHeader,
                                     const QString &exportMacro,
                                     const QString &pluginJsonFileName,
                                     int indentation,
                                     bool usePragmaOnce,
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
    if (usePragmaOnce)
        headerStr << "#pragma once\n\n";
    else
        headerStr << "#ifndef " << guard << "\n#define " << guard << "\n\n";

    if (!sharedHeader.isEmpty())
        Utils::writeIncludeFileDirective(sharedHeader, false, headerStr);

    // include base class header
    if (!baseClassName.isEmpty()) {
        Utils::writeIncludeFileDirective(baseClassName, true, headerStr);
        headerStr  << '\n';
    }

    const QString namespaceIndent = Utils::writeOpeningNameSpaces(namespaceList, indent, headerStr);

    // Class declaraction
    if (!namespaceIndent.isEmpty())
        headerStr << '\n';
    headerStr << namespaceIndent << "class ";
    if (t == QtProjectParameters::SharedLibrary && !exportMacro.isEmpty())
        headerStr << exportMacro << ' ';

    headerStr << unqualifiedClassName;
    if (!baseClassName.isEmpty())
        headerStr << " : public " << baseClassName;
    headerStr << "\n{\n";

    // Is this a QObject (plugin)
    const bool inheritsQObject = t == QtProjectParameters::QtPlugin;
    if (inheritsQObject)
        headerStr << namespaceIndent << indent << "Q_OBJECT\n";
    if (t == QtProjectParameters::QtPlugin) { // Write Qt plugin meta data.
        const QString qt5InterfaceName = LibraryWizardDialog::pluginInterface(baseClassName);
        QTC_CHECK(!qt5InterfaceName.isEmpty());
        headerStr << namespaceIndent << indent << "Q_PLUGIN_METADATA(IID \""
                  << qt5InterfaceName << '"';
        QTC_CHECK(!pluginJsonFileName.isEmpty());
        headerStr << " FILE \"" << pluginJsonFileName << '"';
        headerStr << ")\n";
    }

    headerStr << namespaceIndent << "\npublic:\n";
    if (inheritsQObject) {
        headerStr << namespaceIndent << indent << "explicit " << unqualifiedClassName
                  << "(QObject *parent = nullptr);\n";
    } else {
        headerStr << namespaceIndent << indent << unqualifiedClassName << "();\n";
    }
    if (!pureVirtualSignatures.empty()) {
        headerStr << "\nprivate:\n";
        for (const QString &signature : pureVirtualSignatures)
            headerStr << namespaceIndent << indent << signature << " override;\n";
    }
    headerStr << namespaceIndent << "};\n";
    if (!namespaceIndent.isEmpty())
        headerStr << '\n';
    Utils::writeClosingNameSpaces(namespaceList, indent, headerStr);
    if (!usePragmaOnce)
        headerStr <<  "\n#endif // " << guard << '\n';

    /// 2) Source
    QTextStream sourceStr(source);

    Utils::writeIncludeFileDirective(headerName, false, sourceStr);
    sourceStr << '\n';

    Utils::writeOpeningNameSpaces(namespaceList, indent, sourceStr);
    if (!namespaceIndent.isEmpty())
        sourceStr << '\n';

    // Constructor
    sourceStr << namespaceIndent << unqualifiedClassName << "::" << unqualifiedClassName;
    if (inheritsQObject) {
         sourceStr << "(QObject *parent) :\n"
                   << namespaceIndent << indent << baseClassName << "(parent)\n";
    } else {
        sourceStr << "()\n";
    }
    sourceStr << namespaceIndent << "{\n" << namespaceIndent <<  "}\n";
    for (const QString &signature : pureVirtualSignatures) {
        const int parenIndex = signature.indexOf('(');
        QTC_ASSERT(parenIndex != -1, continue);
        int nameIndex = -1;
        for (int i = parenIndex - 1; i > 0; --i) {
            if (!signature.at(i).isLetterOrNumber()) {
                nameIndex = i + 1;
                break;
            }
        }
        QTC_ASSERT(nameIndex != -1, continue);
        sourceStr << '\n' << namespaceIndent << signature.left(nameIndex);
        if (!std::strchr("&* ", signature.at(nameIndex - 1).toLatin1()))
            sourceStr << ' ';
        sourceStr << unqualifiedClassName << "::" << signature.mid(nameIndex) << '\n';
        sourceStr << namespaceIndent << "{\n" << indent
                  << "static_assert(false, \"You need to implement this function\");\n}\n";
    }

    Utils::writeClosingNameSpaces(namespaceList, indent, sourceStr);
}

QString  LibraryParameters::generateSharedHeader(const QString &globalHeaderFileName,
                                                 const QString &projectTarget,
                                                 const QString &exportMacro,
                                                 bool usePragmaOnce)
{
    QString contents;
    if (usePragmaOnce) {
        contents += "#pragma once\n";
    } else {
        contents += "#ifndef " + Utils::headerGuard(globalHeaderFileName) + "\n";
        contents += "#define " + Utils::headerGuard(globalHeaderFileName) + "\n";
    }
    contents += "\n";
    contents += "#include <QtCore/qglobal.h>\n";
    contents += "\n";
    contents += "#if defined(" + QtProjectParameters::libraryMacro(projectTarget) + ")\n";
    contents += "#  define " + exportMacro + " Q_DECL_EXPORT\n";
    contents += "#else\n";
    contents += "#  define " + exportMacro + " Q_DECL_IMPORT\n";
    contents += "#endif\n";
    contents += "\n";
    if (!usePragmaOnce)
        contents += "#endif // " + Utils::headerGuard(globalHeaderFileName) + '\n';

    return contents;
}

} // namespace Internal
} // namespace QmakeProjectManager
