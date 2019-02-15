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

#include "clangtoolslogfilereader.h"

#include <cpptools/cppprojectfile.h>

#include <QDebug>
#include <QDir>
#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <utils/executeondestruction.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <clang-c/Index.h>

namespace ClangTools {
namespace Internal {

static QString fromCXString(CXString &&cxString)
{
    QString result = QString::fromUtf8(clang_getCString(cxString));
    clang_disposeString(cxString);
    return result;
}

static Debugger::DiagnosticLocation diagLocationFromSourceLocation(CXSourceLocation cxLocation)
{
    CXFile file;
    unsigned line;
    unsigned column;
    clang_getSpellingLocation(cxLocation, &file, &line, &column, nullptr);

    Debugger::DiagnosticLocation location;
    location.filePath = fromCXString(clang_getFileName(file));
    location.filePath = QDir::cleanPath(location.filePath); // Normalize to find duplicates later
    location.line = line;
    location.column = column;
    return location;
}

static QString cxDiagnosticType(const CXDiagnostic cxDiagnostic)
{
    const CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(cxDiagnostic);
    switch (severity) {
    case CXDiagnostic_Note:
        return QString("note");
    case CXDiagnostic_Warning:
        return QString("warning");
    case CXDiagnostic_Error:
        return QString("error");
    case CXDiagnostic_Fatal:
        return QString("fatal");
    case CXDiagnostic_Ignored:
        return QString("ignored");
    }
    return QString("ignored");
}

static ExplainingStep buildChildDiagnostic(const CXDiagnostic cxDiagnostic)
{
    ExplainingStep diagnosticStep;
    QString type = cxDiagnosticType(cxDiagnostic);
    if (type == QStringLiteral("ignored"))
        return diagnosticStep;

    const CXSourceLocation cxLocation = clang_getDiagnosticLocation(cxDiagnostic);
    diagnosticStep.location = diagLocationFromSourceLocation(cxLocation);
    diagnosticStep.message = fromCXString(clang_getDiagnosticSpelling(cxDiagnostic));
    return diagnosticStep;
}

static bool isInvalidDiagnosticLocation(const Diagnostic &diagnostic, const ExplainingStep &child,
                                        const QString &nativeFilePath)
{
    // When main file is considered included by itself - this diagnostic has invalid location.
    // This case usually happens when original diagnostic comes from system header but
    // has main file name set in the source location instead (which is incorrect).
    return child.message.indexOf(nativeFilePath) >= 0
            && child.message.indexOf("in file included from") >= 0
            && diagnostic.location.filePath == nativeFilePath;
}

static ExplainingStep buildFixIt(const CXDiagnostic cxDiagnostic, unsigned index)
{
    ExplainingStep fixItStep;
    CXSourceRange cxFixItRange;
    fixItStep.isFixIt = true;
    fixItStep.message = fromCXString(clang_getDiagnosticFixIt(cxDiagnostic, index, &cxFixItRange));
    fixItStep.location = diagLocationFromSourceLocation(clang_getRangeStart(cxFixItRange));
    fixItStep.ranges.push_back(fixItStep.location);
    fixItStep.ranges.push_back(diagLocationFromSourceLocation(clang_getRangeEnd(cxFixItRange)));
    return fixItStep;
}

static Diagnostic buildDiagnostic(const CXDiagnostic cxDiagnostic,
                                  const Utils::FileName &projectRootDir,
                                  const QString &nativeFilePath)
{
    Diagnostic diagnostic;
    diagnostic.type = cxDiagnosticType(cxDiagnostic);
    if (diagnostic.type == QStringLiteral("ignored"))
        return diagnostic;

    const CXSourceLocation cxLocation = clang_getDiagnosticLocation(cxDiagnostic);
    if (clang_Location_isInSystemHeader(cxLocation))
        return diagnostic;

    diagnostic.location = diagLocationFromSourceLocation(cxLocation);
    const auto diagnosticFilePath = Utils::FileName::fromString(diagnostic.location.filePath);
    if (!diagnosticFilePath.isChildOf(projectRootDir))
        return diagnostic;

    // TODO: Introduce CppTools::ProjectFile::isGenerated to filter these out properly
    const QString fileName = diagnosticFilePath.fileName();
    if ((fileName.startsWith("ui_") && fileName.endsWith(".h")) || fileName.endsWith(".moc"))
        return diagnostic;

    CXDiagnosticSet cxChildDiagnostics = clang_getChildDiagnostics(cxDiagnostic);
    Utils::ExecuteOnDestruction onBuildExit([&]() {
        clang_disposeDiagnosticSet(cxChildDiagnostics);
    });

    using CppTools::ProjectFile;
    const bool isHeaderFile = ProjectFile::isHeader(
        ProjectFile::classify(diagnostic.location.filePath));

    for (unsigned i = 0; i < clang_getNumDiagnosticsInSet(cxChildDiagnostics); ++i) {
        CXDiagnostic cxDiagnostic = clang_getDiagnosticInSet(cxChildDiagnostics, i);
        Utils::ExecuteOnDestruction cleanUpDiagnostic([&]() {
            clang_disposeDiagnostic(cxDiagnostic);
        });
        const ExplainingStep diagnosticStep = buildChildDiagnostic(cxDiagnostic);
        if (diagnosticStep.isValid())
            continue;

        if (isHeaderFile && diagnosticStep.message.contains("in file included from"))
            continue;

        if (isInvalidDiagnosticLocation(diagnostic, diagnosticStep, nativeFilePath))
            return diagnostic;

        diagnostic.explainingSteps.push_back(diagnosticStep);
    }

    const unsigned fixItCount = clang_getDiagnosticNumFixIts(cxDiagnostic);
    diagnostic.hasFixits = fixItCount != 0;
    for (unsigned i = 0; i < fixItCount; ++i)
        diagnostic.explainingSteps.push_back(buildFixIt(cxDiagnostic, i));

    diagnostic.description = fromCXString(clang_getDiagnosticSpelling(cxDiagnostic));
    diagnostic.category = fromCXString(clang_getDiagnosticCategoryText(cxDiagnostic));

    return diagnostic;
}

static QList<Diagnostic> readSerializedDiagnostics_helper(const QString &filePath,
                                                          const Utils::FileName &projectRootDir,
                                                          const QString &logFilePath)
{
    QList<Diagnostic> list;
    CXLoadDiag_Error error;
    CXString errorString;

    CXDiagnosticSet diagnostics = clang_loadDiagnostics(logFilePath.toStdString().c_str(),
                                                        &error,
                                                        &errorString);
    if (error != CXLoadDiag_None || !diagnostics)
        return list;

    Utils::ExecuteOnDestruction onReadExit([&]() {
        clang_disposeDiagnosticSet(diagnostics);
    });

    const QString nativeFilePath = QDir::toNativeSeparators(filePath);
    for (unsigned i = 0; i < clang_getNumDiagnosticsInSet(diagnostics); ++i) {
        CXDiagnostic cxDiagnostic = clang_getDiagnosticInSet(diagnostics, i);
        Utils::ExecuteOnDestruction cleanUpDiagnostic([&]() {
            clang_disposeDiagnostic(cxDiagnostic);
        });
        const Diagnostic diagnostic = buildDiagnostic(cxDiagnostic, projectRootDir, nativeFilePath);
        if (!diagnostic.isValid())
            continue;

        list.push_back(diagnostic);
    }

    return list;
}

static bool checkFilePath(const QString &filePath, QString *errorMessage)
{
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isReadable()) {
        if (errorMessage) {
            *errorMessage
                    = QString(QT_TRANSLATE_NOOP("LogFileReader",
                                                "File \"%1\" does not exist or is not readable."))
                    .arg(filePath);
        }
        return false;
    }
    return true;
}

QList<Diagnostic> readSerializedDiagnostics(const QString &filePath,
                                            const Utils::FileName &projectRootDir,
                                            const QString &logFilePath,
                                            QString *errorMessage)
{
    if (!checkFilePath(logFilePath, errorMessage))
        return QList<Diagnostic>();

    return readSerializedDiagnostics_helper(filePath, projectRootDir, logFilePath);
}

} // namespace Internal
} // namespace ClangTools
