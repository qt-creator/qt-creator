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

class ClangStaticAnalyzerLogFileReader
{
    Q_DECLARE_TR_FUNCTIONS(ClangTools::Internal::ClangStaticAnalyzerLogFileReader)
public:
    ClangStaticAnalyzerLogFileReader(const QString &filePath);

    QXmlStreamReader::Error read();

    // Output
    QString clangVersion() const;
    QStringList files() const;
    QList<Diagnostic> diagnostics() const;

private:
    void readPlist();
    void readTopLevelDict();
    void readDiagnosticsArray();
    void readDiagnosticsDict();
    QList<ExplainingStep> readPathArray();
    ExplainingStep readPathDict();
    Debugger::DiagnosticLocation readLocationDict(bool elementIsRead = false);
    QList<Debugger::DiagnosticLocation> readRangesArray();

    QString readString();
    QStringList readStringArray();
    int readInteger(bool *convertedSuccessfully);

private:
    QString m_filePath;
    QXmlStreamReader m_xml;

    QString m_clangVersion;
    QStringList m_referencedFiles;
    QList<Diagnostic> m_diagnostics;
};

class ClangSerializedDiagnosticsReader
{
public:
    QList<Diagnostic> read(const QString &filePath, const QString &logFilePath);
};

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

QList<Diagnostic> LogFileReader::readPlist(const QString &filePath, QString *errorMessage)
{
    if (!checkFilePath(filePath, errorMessage))
        return QList<Diagnostic>();

    // Read
    ClangStaticAnalyzerLogFileReader reader(filePath);
    const QXmlStreamReader::Error error = reader.read();

    // Return diagnostics
    switch (error) {
    case QXmlStreamReader::NoError:
        return reader.diagnostics();

    // Handle errors
    case QXmlStreamReader::UnexpectedElementError:
        if (errorMessage) {
            *errorMessage = tr("Could not read file \"%1\": UnexpectedElementError.")
                                .arg(filePath);
        }
        Q_FALLTHROUGH();
    case QXmlStreamReader::CustomError:
        if (errorMessage) {
            *errorMessage = tr("Could not read file \"%1\": CustomError.")
                                .arg(filePath);
        }
        Q_FALLTHROUGH();
    case QXmlStreamReader::NotWellFormedError:
        if (errorMessage) {
            *errorMessage = tr("Could not read file \"%1\": NotWellFormedError.")
                                .arg(filePath);
        }
        Q_FALLTHROUGH();
    case QXmlStreamReader::PrematureEndOfDocumentError:
        if (errorMessage) {
            *errorMessage = tr("Could not read file \"%1\": PrematureEndOfDocumentError.")
                                .arg(filePath);
        }
        Q_FALLTHROUGH();
    default:
        return QList<Diagnostic>();
    }
}

QList<Diagnostic> LogFileReader::readSerialized(const QString &filePath, const QString &logFilePath,
                                                QString *errorMessage)
{
    if (!checkFilePath(filePath, errorMessage))
        return QList<Diagnostic>();

    ClangSerializedDiagnosticsReader reader;
    return reader.read(filePath, logFilePath);
}

ClangStaticAnalyzerLogFileReader::ClangStaticAnalyzerLogFileReader(const QString &filePath)
    : m_filePath(filePath)
{
}

QXmlStreamReader::Error ClangStaticAnalyzerLogFileReader::read()
{
    QTC_ASSERT(!m_filePath.isEmpty(), return QXmlStreamReader::CustomError);
    QFile file(m_filePath);
    QTC_ASSERT(file.open(QIODevice::ReadOnly | QIODevice::Text),
               return QXmlStreamReader::CustomError);

    m_xml.setDevice(&file);
    readPlist();

    // If file is empty, m_xml.error() == QXmlStreamReader::PrematureEndOfDocumentError
    return m_xml.error();
}

QString ClangStaticAnalyzerLogFileReader::clangVersion() const
{
    return m_clangVersion;
}

QStringList ClangStaticAnalyzerLogFileReader::files() const
{
    return m_referencedFiles;
}

QList<Diagnostic> ClangStaticAnalyzerLogFileReader::diagnostics() const
{
    return m_diagnostics;
}

void ClangStaticAnalyzerLogFileReader::readPlist()
{
    if (m_xml.readNextStartElement()) {
        if (m_xml.name() == QLatin1String("plist")) {
            if (m_xml.attributes().value(QLatin1String("version")) == QLatin1String("1.0"))
                readTopLevelDict();
        } else {
            m_xml.raiseError(tr("File is not a plist version 1.0 file."));
        }
    }
}

void ClangStaticAnalyzerLogFileReader::readTopLevelDict()
{
    QTC_ASSERT(m_xml.isStartElement() && m_xml.name() == QLatin1String("plist"), return);
    QTC_ASSERT(m_xml.readNextStartElement() && m_xml.name() == QLatin1String("dict"), return);

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == QLatin1String("key")) {
            const QString key = m_xml.readElementText();
            if (key == QLatin1String("clang_version"))
                m_clangVersion = readString();
            else if (key == QLatin1String("files"))
                m_referencedFiles = readStringArray();
            else if (key == QLatin1String("diagnostics"))
                readDiagnosticsArray();
        } else {
            m_xml.skipCurrentElement();
        }
    }
}

void ClangStaticAnalyzerLogFileReader::readDiagnosticsArray()
{
    if (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("array")) {
        while (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("dict"))
            readDiagnosticsDict();
    }
}

void ClangStaticAnalyzerLogFileReader::readDiagnosticsDict()
{
    QTC_ASSERT(m_xml.isStartElement() && m_xml.name() == QLatin1String("dict"), return);

    Diagnostic diagnostic;

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == QLatin1String("key")) {
            const QString key = m_xml.readElementText();
            if (key == QLatin1String("path"))
                diagnostic.explainingSteps = readPathArray();
            else if (key == QLatin1String("description"))
                diagnostic.description = readString();
            else if (key == QLatin1String("category"))
                diagnostic.category = readString();
            else if (key == QLatin1String("type"))
                diagnostic.type = readString();
            else if (key == QLatin1String("issue_context_kind"))
                diagnostic.issueContextKind = readString();
            else if (key == QLatin1String("issue_context"))
                diagnostic.issueContext = readString();
            else if (key == QLatin1String("location"))
                diagnostic.location = readLocationDict();
        } else {
            m_xml.skipCurrentElement();
        }
    }

    m_diagnostics << diagnostic;
}

QList<ExplainingStep> ClangStaticAnalyzerLogFileReader::readPathArray()
{
    QList<ExplainingStep> result;

    if (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("array")) {
        while (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("dict")) {
            const ExplainingStep step = readPathDict();
            if (step.isValid())
                result << step;
        }
    }

    return result;
}

ExplainingStep ClangStaticAnalyzerLogFileReader::readPathDict()
{
    ExplainingStep explainingStep;

    QTC_ASSERT(m_xml.isStartElement() && m_xml.name() == QLatin1String("dict"),
               return explainingStep);

    // We are interested only in dict entries an kind=event type
    if (m_xml.readNextStartElement()) {
        if (m_xml.name() == QLatin1String("key")) {
            const QString key = m_xml.readElementText();
            QTC_ASSERT(key == QLatin1String("kind"), return explainingStep);
            const QString kind = readString();
            if (kind != QLatin1String("event")) {
                m_xml.skipCurrentElement();
                return explainingStep;
            }
        }
    }

    bool depthOk = false;

    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == QLatin1String("key")) {
            const QString key = m_xml.readElementText();
            if (key == QLatin1String("location"))
                explainingStep.location = readLocationDict();
            else if (key == QLatin1String("ranges"))
                explainingStep.ranges = readRangesArray();
            else if (key == QLatin1String("depth"))
                explainingStep.depth = readInteger(&depthOk);
            else if (key == QLatin1String("message"))
                explainingStep.message = readString();
            else if (key == QLatin1String("extended_message"))
                explainingStep.extendedMessage = readString();
        } else {
            m_xml.skipCurrentElement();
        }
    }

    QTC_CHECK(depthOk);
    return explainingStep;
}

Debugger::DiagnosticLocation ClangStaticAnalyzerLogFileReader::readLocationDict(bool elementIsRead)
{
    Debugger::DiagnosticLocation location;
    if (elementIsRead) {
        QTC_ASSERT(m_xml.isStartElement() && m_xml.name() == QLatin1String("dict"),
                   return location);
    } else {
        QTC_ASSERT(m_xml.readNextStartElement() && m_xml.name() == QLatin1String("dict"),
                   return location);
    }

    int line = 0;
    int column = 0;
    int fileIndex = 0;
    bool lineOk = false, columnOk = false, fileIndexOk = false;

    // Collect values
    while (m_xml.readNextStartElement()) {
        if (m_xml.name() == QLatin1String("key")) {
            const QString keyName = m_xml.readElementText();
            if (keyName == QLatin1String("line"))
                line = readInteger(&lineOk);
            else if (keyName == QLatin1String("col"))
                column = readInteger(&columnOk);
            else if (keyName == QLatin1String("file"))
                fileIndex = readInteger(&fileIndexOk);
        } else {
            m_xml.skipCurrentElement();
        }
    }

    if (lineOk && columnOk && fileIndexOk) {
        QTC_ASSERT(fileIndex < m_referencedFiles.size(), return location);
        location = Debugger::DiagnosticLocation(m_referencedFiles.at(fileIndex), line, column);
    }
    return location;
}

QList<Debugger::DiagnosticLocation> ClangStaticAnalyzerLogFileReader::readRangesArray()
{
    QList<Debugger::DiagnosticLocation> result;

    // It's an array of arrays...
    QTC_ASSERT(m_xml.readNextStartElement() && m_xml.name() == QLatin1String("array"),
               return result);
    QTC_ASSERT(m_xml.readNextStartElement() && m_xml.name() == QLatin1String("array"),
               return result);

    while (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("dict"))
        result << readLocationDict(true);

    m_xml.skipCurrentElement(); // Laeve outer array
    return result;
}

QString ClangStaticAnalyzerLogFileReader::readString()
{
    if (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("string"))
        return m_xml.readElementText();

    m_xml.raiseError(tr("Expected a string element."));
    return QString();
}

QStringList ClangStaticAnalyzerLogFileReader::readStringArray()
{
    if (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("array")) {
        QStringList result;
        while (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("string")) {
            const QString string = m_xml.readElementText();
            if (!string.isEmpty())
                result << string;
        }
        return result;
    }

    m_xml.raiseError(tr("Expected an array element."));
    return QStringList();
}

int ClangStaticAnalyzerLogFileReader::readInteger(bool *convertedSuccessfully)
{
    if (m_xml.readNextStartElement() && m_xml.name() == QLatin1String("integer")) {
        const QString contents = m_xml.readElementText();
        return contents.toInt(convertedSuccessfully);
    }

    m_xml.raiseError(tr("Expected an integer element."));
    if (convertedSuccessfully)
        *convertedSuccessfully = false;
    return -1;
}

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
    diagnosticStep.message = type + ": " + fromCXString(clang_getDiagnosticSpelling(cxDiagnostic));
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
    fixItStep.message = "fix-it: " + fromCXString(clang_getDiagnosticFixIt(cxDiagnostic, index,
                                                                           &cxFixItRange));
    fixItStep.location = diagLocationFromSourceLocation(clang_getRangeStart(cxFixItRange));
    fixItStep.ranges.push_back(fixItStep.location);
    fixItStep.ranges.push_back(diagLocationFromSourceLocation(clang_getRangeEnd(cxFixItRange)));
    return fixItStep;
}

static Diagnostic buildDiagnostic(const CXDiagnostic cxDiagnostic, const QString &nativeFilePath)
{
    Diagnostic diagnostic;
    diagnostic.type = cxDiagnosticType(cxDiagnostic);
    if (diagnostic.type == QStringLiteral("ignored"))
        return diagnostic;

    const CXSourceLocation cxLocation = clang_getDiagnosticLocation(cxDiagnostic);
    if (clang_Location_isInSystemHeader(cxLocation))
        return diagnostic;

    diagnostic.location = diagLocationFromSourceLocation(cxLocation);
    if (diagnostic.location.filePath != nativeFilePath)
        return diagnostic;

    CXDiagnosticSet cxChildDiagnostics = clang_getChildDiagnostics(cxDiagnostic);
    Utils::ExecuteOnDestruction onBuildExit([&]() {
        clang_disposeDiagnosticSet(cxChildDiagnostics);
    });

    for (unsigned i = 0; i < clang_getNumDiagnosticsInSet(cxChildDiagnostics); ++i) {
        CXDiagnostic cxDiagnostic = clang_getDiagnosticInSet(cxChildDiagnostics, i);
        Utils::ExecuteOnDestruction cleanUpDiagnostic([&]() {
            clang_disposeDiagnostic(cxDiagnostic);
        });
        const ExplainingStep diagnosticStep = buildChildDiagnostic(cxDiagnostic);
        if (diagnosticStep.isValid())
            continue;

        if (isInvalidDiagnosticLocation(diagnostic, diagnosticStep, nativeFilePath))
            return diagnostic;

        diagnostic.explainingSteps.push_back(diagnosticStep);
    }

    for (unsigned i = 0; i < clang_getDiagnosticNumFixIts(cxDiagnostic); ++i)
        diagnostic.explainingSteps.push_back(buildFixIt(cxDiagnostic, i));

    diagnostic.description = fromCXString(clang_getDiagnosticSpelling(cxDiagnostic));
    diagnostic.category = fromCXString(clang_getDiagnosticCategoryText(cxDiagnostic));

    return diagnostic;
}

QList<Diagnostic> ClangSerializedDiagnosticsReader::read(const QString &filePath,
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
        const Diagnostic diagnostic = buildDiagnostic(cxDiagnostic, nativeFilePath);
        if (!diagnostic.isValid())
            continue;

        list.push_back(diagnostic);
    }

    return list;
}

} // namespace Internal
} // namespace ClangTools
