/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "cppcodemodelinspectordialog.h"
#include "cppeditor.h"
#include "ui_cppcodemodelinspectordialog.h"

#include <app/app_version.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cppprojectfile.h>
#include <cpptools/cpptoolseditorsupport.h>
#include <projectexplorer/project.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Token.h>
#include <utils/qtcassert.h>

#include <QAbstractTableModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>

using namespace CppTools;

namespace {

// --- Utils --------------------------------------------------------------------------------------

QString toString(bool value)
{
    return value ? QLatin1String("Yes") : QLatin1String("No");
}

QString toString(unsigned value)
{
    return QString::number(value);
}

QString toString(const QDateTime &dateTime)
{
    return dateTime.toString(QLatin1String("hh:mm:ss dd.MM.yy"));
}

QString toString(CPlusPlus::Document::CheckMode checkMode)
{
#define CASE_CHECKMODE(x) case CPlusPlus::Document::x: return QLatin1String(#x)
    switch (checkMode) {
    CASE_CHECKMODE(Unchecked);
    CASE_CHECKMODE(FullCheck);
    CASE_CHECKMODE(FastCheck);
    // no default to get a compiler warning if anything is added
    }
#undef CASE_CHECKMODE
    return QString();
}

QString toString(CPlusPlus::Document::DiagnosticMessage::Level level)
{
#define CASE_LEVEL(x) case CPlusPlus::Document::DiagnosticMessage::x: return QLatin1String(#x)
    switch (level) {
    CASE_LEVEL(Warning);
    CASE_LEVEL(Error);
    CASE_LEVEL(Fatal);
    // no default to get a compiler warning if anything is added
    }
#undef CASE_LEVEL
    return QString();
}

QString toString(ProjectPart::CVersion cVersion)
{
#define CASE_CVERSION(x) case ProjectPart::x: return QLatin1String(#x)
    switch (cVersion) {
    CASE_CVERSION(C89);
    CASE_CVERSION(C99);
    CASE_CVERSION(C11);
    // no default to get a compiler warning if anything is added
    }
#undef CASE_CVERSION
    return QString();
}

QString toString(ProjectPart::CXXVersion cxxVersion)
{
#define CASE_CXXVERSION(x) case ProjectPart::x: return QLatin1String(#x)
    switch (cxxVersion) {
    CASE_CXXVERSION(CXX98);
    CASE_CXXVERSION(CXX11);
    // no default to get a compiler warning if anything is added
    }
#undef CASE_CXXVERSION
    return QString();
}

QString toString(ProjectPart::CXXExtensions cxxExtension)
{
    QString result;

#define CASE_CXXEXTENSION(ext) if (cxxExtension & ProjectPart::ext) \
    result += QLatin1String(#ext ", ");

    CASE_CXXEXTENSION(NoExtensions);
    CASE_CXXEXTENSION(GnuExtensions);
    CASE_CXXEXTENSION(MicrosoftExtensions);
    CASE_CXXEXTENSION(BorlandExtensions);
    CASE_CXXEXTENSION(OpenMPExtensions);
#undef CASE_CXXEXTENSION
    if (result.endsWith(QLatin1String(", ")))
        result.chop(2);
    return result;
}

QString toString(ProjectPart::QtVersion qtVersion)
{
#define CASE_QTVERSION(x) case ProjectPart::x: return QLatin1String(#x)
    switch (qtVersion) {
    CASE_QTVERSION(UnknownQt);
    CASE_QTVERSION(NoQt);
    CASE_QTVERSION(Qt4);
    CASE_QTVERSION(Qt5);
    // no default to get a compiler warning if anything is added
    }
#undef CASE_QTVERSION
    return QString();
}

QString toString(const QList<ProjectFile> &projectFiles)
{
    QStringList filesList;
    foreach (const ProjectFile &projectFile, projectFiles)
        filesList << QDir::toNativeSeparators(projectFile.path);
    qSort(filesList);
    return filesList.join(QLatin1String("\n"));
}

QString toString(ProjectFile::Kind kind)
{
#define CASE_PROFECTFILEKIND(x) case ProjectFile::x: return QLatin1String(#x)
    switch (kind) {
    CASE_PROFECTFILEKIND(Unclassified);
    CASE_PROFECTFILEKIND(CHeader);
    CASE_PROFECTFILEKIND(CSource);
    CASE_PROFECTFILEKIND(CXXHeader);
    CASE_PROFECTFILEKIND(CXXSource);
    CASE_PROFECTFILEKIND(ObjCHeader);
    CASE_PROFECTFILEKIND(ObjCSource);
    CASE_PROFECTFILEKIND(ObjCXXHeader);
    CASE_PROFECTFILEKIND(ObjCXXSource);
    CASE_PROFECTFILEKIND(CudaSource);
    CASE_PROFECTFILEKIND(OpenCLSource);
    // no default to get a compiler warning if anything is added
    }
#undef CASE_PROFECTFILEKIND
    return QString();
}

QString toString(CPlusPlus::Kind kind)
{
    using namespace CPlusPlus;
#define TOKEN(x) case x: return QLatin1String(#x)
#define TOKEN_AND_ALIASES(x,y) case x: return QLatin1String(#x "/" #y)
    switch (kind) {
    TOKEN(T_EOF_SYMBOL);
    TOKEN(T_ERROR);
    TOKEN(T_CPP_COMMENT);
    TOKEN(T_CPP_DOXY_COMMENT);
    TOKEN(T_COMMENT);
    TOKEN(T_DOXY_COMMENT);
    TOKEN(T_IDENTIFIER);
    TOKEN(T_NUMERIC_LITERAL);
    TOKEN(T_CHAR_LITERAL);
    TOKEN(T_WIDE_CHAR_LITERAL);
    TOKEN(T_UTF16_CHAR_LITERAL);
    TOKEN(T_UTF32_CHAR_LITERAL);
    TOKEN(T_STRING_LITERAL);
    TOKEN(T_WIDE_STRING_LITERAL);
    TOKEN(T_UTF8_STRING_LITERAL);
    TOKEN(T_UTF16_STRING_LITERAL);
    TOKEN(T_UTF32_STRING_LITERAL);
    TOKEN(T_RAW_STRING_LITERAL);
    TOKEN(T_RAW_WIDE_STRING_LITERAL);
    TOKEN(T_RAW_UTF8_STRING_LITERAL);
    TOKEN(T_RAW_UTF16_STRING_LITERAL);
    TOKEN(T_RAW_UTF32_STRING_LITERAL);
    TOKEN(T_AT_STRING_LITERAL);
    TOKEN(T_ANGLE_STRING_LITERAL);
    TOKEN_AND_ALIASES(T_AMPER, T_BITAND);
    TOKEN_AND_ALIASES(T_AMPER_AMPER, T_AND);
    TOKEN_AND_ALIASES(T_AMPER_EQUAL, T_AND_EQ);
    TOKEN(T_ARROW);
    TOKEN(T_ARROW_STAR);
    TOKEN_AND_ALIASES(T_CARET, T_XOR);
    TOKEN_AND_ALIASES(T_CARET_EQUAL, T_XOR_EQ);
    TOKEN(T_COLON);
    TOKEN(T_COLON_COLON);
    TOKEN(T_COMMA);
    TOKEN(T_SLASH);
    TOKEN(T_SLASH_EQUAL);
    TOKEN(T_DOT);
    TOKEN(T_DOT_DOT_DOT);
    TOKEN(T_DOT_STAR);
    TOKEN(T_EQUAL);
    TOKEN(T_EQUAL_EQUAL);
    TOKEN_AND_ALIASES(T_EXCLAIM, T_NOT);
    TOKEN_AND_ALIASES(T_EXCLAIM_EQUAL, T_NOT_EQ);
    TOKEN(T_GREATER);
    TOKEN(T_GREATER_EQUAL);
    TOKEN(T_GREATER_GREATER);
    TOKEN(T_GREATER_GREATER_EQUAL);
    TOKEN(T_LBRACE);
    TOKEN(T_LBRACKET);
    TOKEN(T_LESS);
    TOKEN(T_LESS_EQUAL);
    TOKEN(T_LESS_LESS);
    TOKEN(T_LESS_LESS_EQUAL);
    TOKEN(T_LPAREN);
    TOKEN(T_MINUS);
    TOKEN(T_MINUS_EQUAL);
    TOKEN(T_MINUS_MINUS);
    TOKEN(T_PERCENT);
    TOKEN(T_PERCENT_EQUAL);
    TOKEN_AND_ALIASES(T_PIPE, T_BITOR);
    TOKEN_AND_ALIASES(T_PIPE_EQUAL, T_OR_EQ);
    TOKEN_AND_ALIASES(T_PIPE_PIPE, T_OR);
    TOKEN(T_PLUS);
    TOKEN(T_PLUS_EQUAL);
    TOKEN(T_PLUS_PLUS);
    TOKEN(T_POUND);
    TOKEN(T_POUND_POUND);
    TOKEN(T_QUESTION);
    TOKEN(T_RBRACE);
    TOKEN(T_RBRACKET);
    TOKEN(T_RPAREN);
    TOKEN(T_SEMICOLON);
    TOKEN(T_STAR);
    TOKEN(T_STAR_EQUAL);
    TOKEN_AND_ALIASES(T_TILDE, T_COMPL);
    TOKEN(T_TILDE_EQUAL);
    TOKEN(T_ALIGNAS);
    TOKEN(T_ALIGNOF);
    TOKEN_AND_ALIASES(T_ASM, T___ASM/T___ASM__);
    TOKEN(T_AUTO);
    TOKEN(T_BOOL);
    TOKEN(T_BREAK);
    TOKEN(T_CASE);
    TOKEN(T_CATCH);
    TOKEN(T_CHAR);
    TOKEN(T_CHAR16_T);
    TOKEN(T_CHAR32_T);
    TOKEN(T_CLASS);
    TOKEN_AND_ALIASES(T_CONST, T___CONST/T___CONST__);
    TOKEN(T_CONST_CAST);
    TOKEN(T_CONSTEXPR);
    TOKEN(T_CONTINUE);
    TOKEN_AND_ALIASES(T_DECLTYPE, T___DECLTYPE);
    TOKEN(T_DEFAULT);
    TOKEN(T_DELETE);
    TOKEN(T_DO);
    TOKEN(T_DOUBLE);
    TOKEN(T_DYNAMIC_CAST);
    TOKEN(T_ELSE);
    TOKEN(T_ENUM);
    TOKEN(T_EXPLICIT);
    TOKEN(T_EXPORT);
    TOKEN(T_EXTERN);
    TOKEN(T_FALSE);
    TOKEN(T_FLOAT);
    TOKEN(T_FOR);
    TOKEN(T_FRIEND);
    TOKEN(T_GOTO);
    TOKEN(T_IF);
    TOKEN_AND_ALIASES(T_INLINE, T___INLINE/T___INLINE__);
    TOKEN(T_INT);
    TOKEN(T_LONG);
    TOKEN(T_MUTABLE);
    TOKEN(T_NAMESPACE);
    TOKEN(T_NEW);
    TOKEN(T_NOEXCEPT);
    TOKEN(T_NULLPTR);
    TOKEN(T_OPERATOR);
    TOKEN(T_PRIVATE);
    TOKEN(T_PROTECTED);
    TOKEN(T_PUBLIC);
    TOKEN(T_REGISTER);
    TOKEN(T_REINTERPRET_CAST);
    TOKEN(T_RETURN);
    TOKEN(T_SHORT);
    TOKEN(T_SIGNED);
    TOKEN(T_SIZEOF);
    TOKEN(T_STATIC);
    TOKEN(T_STATIC_ASSERT);
    TOKEN(T_STATIC_CAST);
    TOKEN(T_STRUCT);
    TOKEN(T_SWITCH);
    TOKEN(T_TEMPLATE);
    TOKEN(T_THIS);
    TOKEN(T_THREAD_LOCAL);
    TOKEN(T_THROW);
    TOKEN(T_TRUE);
    TOKEN(T_TRY);
    TOKEN(T_TYPEDEF);
    TOKEN(T_TYPEID);
    TOKEN(T_TYPENAME);
    TOKEN(T_UNION);
    TOKEN(T_UNSIGNED);
    TOKEN(T_USING);
    TOKEN(T_VIRTUAL);
    TOKEN(T_VOID);
    TOKEN_AND_ALIASES(T_VOLATILE, T___VOLATILE/T___VOLATILE__);
    TOKEN(T_WCHAR_T);
    TOKEN(T_WHILE);
    TOKEN_AND_ALIASES(T___ATTRIBUTE__, T___ATTRIBUTE);
    TOKEN(T___THREAD);
    TOKEN_AND_ALIASES(T___TYPEOF__, T_TYPEOF/T___TYPEOF);
    TOKEN(T_AT_CATCH);
    TOKEN(T_AT_CLASS);
    TOKEN(T_AT_COMPATIBILITY_ALIAS);
    TOKEN(T_AT_DEFS);
    TOKEN(T_AT_DYNAMIC);
    TOKEN(T_AT_ENCODE);
    TOKEN(T_AT_END);
    TOKEN(T_AT_FINALLY);
    TOKEN(T_AT_IMPLEMENTATION);
    TOKEN(T_AT_INTERFACE);
    TOKEN(T_AT_NOT_KEYWORD);
    TOKEN(T_AT_OPTIONAL);
    TOKEN(T_AT_PACKAGE);
    TOKEN(T_AT_PRIVATE);
    TOKEN(T_AT_PROPERTY);
    TOKEN(T_AT_PROTECTED);
    TOKEN(T_AT_PROTOCOL);
    TOKEN(T_AT_PUBLIC);
    TOKEN(T_AT_REQUIRED);
    TOKEN(T_AT_SELECTOR);
    TOKEN(T_AT_SYNCHRONIZED);
    TOKEN(T_AT_SYNTHESIZE);
    TOKEN(T_AT_THROW);
    TOKEN(T_AT_TRY);
    TOKEN(T_EMIT);
    TOKEN(T_SIGNAL);
    TOKEN(T_SLOT);
    TOKEN(T_Q_SIGNAL);
    TOKEN(T_Q_SLOT);
    TOKEN(T_Q_SIGNALS);
    TOKEN(T_Q_SLOTS);
    TOKEN(T_Q_FOREACH);
    TOKEN(T_Q_D);
    TOKEN(T_Q_Q);
    TOKEN(T_Q_INVOKABLE);
    TOKEN(T_Q_PROPERTY);
    TOKEN(T_Q_PRIVATE_PROPERTY);
    TOKEN(T_Q_INTERFACES);
    TOKEN(T_Q_EMIT);
    TOKEN(T_Q_ENUMS);
    TOKEN(T_Q_FLAGS);
    TOKEN(T_Q_PRIVATE_SLOT);
    TOKEN(T_Q_DECLARE_INTERFACE);
    TOKEN(T_Q_OBJECT);
    TOKEN(T_Q_GADGET);
    // no default to get a compiler warning if anything is added
    }
#undef TOKEN
#undef TOKEN_AND_ALIASES
    return QString();
}

QString partsForFile(const QString &fileName)
{
    const QList<ProjectPart::Ptr> parts
        = CppModelManagerInterface::instance()->projectPart(fileName);
    QString result;
    foreach (const ProjectPart::Ptr &part, parts)
        result += part->displayName + QLatin1Char(',');
    if (result.endsWith(QLatin1Char(',')))
        result.chop(1);
    return result;
}

QString unresolvedFileNameWithDelimiters(const CPlusPlus::Document::Include &include)
{
    const QString unresolvedFileName = include.unresolvedFileName();
    if (include.type() == CPlusPlus::Client::IncludeLocal)
        return QLatin1Char('"') + unresolvedFileName + QLatin1Char('"');
    return QLatin1Char('<') + unresolvedFileName + QLatin1Char('>');
}

QString pathListToString(const QStringList &pathList)
{
    QStringList result;
    foreach (const QString &path, pathList)
        result << QDir::toNativeSeparators(path);
    return result.join(QLatin1String("\n"));
}

QList<CPlusPlus::Document::Ptr> snapshotToList(const CPlusPlus::Snapshot &snapshot)
{
    QList<CPlusPlus::Document::Ptr> documents;
    CPlusPlus::Snapshot::const_iterator it = snapshot.begin(), end = snapshot.end();
    for (; it != end; ++it)
        documents.append(it.value());
    return documents;
}

template <class T> void resizeColumns(QTreeView *view)
{
    for (int column = 0; column < T::ColumnCount - 1; ++column)
        view->resizeColumnToContents(column);
}

TextEditor::BaseTextEditor *currentEditor()
{
    return qobject_cast<TextEditor::BaseTextEditor*>(Core::EditorManager::currentEditor());
}

QString fileInCurrentEditor()
{
    if (TextEditor::BaseTextEditor *editor = currentEditor())
        return editor->document()->filePath();
    return QString();
}

class DepthFinder : public CPlusPlus::SymbolVisitor {
public:
    DepthFinder() : m_symbol(0), m_depth(-1), m_foundDepth(-1), m_stop(false) {}

    int operator()(const CPlusPlus::Document::Ptr &document, CPlusPlus::Symbol *symbol)
    {
        m_symbol = symbol;
        accept(document->globalNamespace());
        return m_foundDepth;
    }

    bool preVisit(CPlusPlus::Symbol *symbol)
    {
        if (m_stop)
            return false;

        if (symbol->asScope()) {
            ++m_depth;
            if (symbol == m_symbol) {
                m_foundDepth = m_depth;
                m_stop = true;
            }
            return true;
        }

        return false;
    }

    void postVisit(CPlusPlus::Symbol *symbol)
    {
        if (symbol->asScope())
            --m_depth;
    }

private:
    CPlusPlus::Symbol *m_symbol;
    int m_depth;
    int m_foundDepth;
    bool m_stop;
};

class CppCodeModelInspectorDumper
{
public:
    explicit CppCodeModelInspectorDumper(const CPlusPlus::Snapshot &globalSnapshot);
    ~CppCodeModelInspectorDumper();

    void dumpProjectInfos(const QList<CppModelManagerInterface::ProjectInfo> &projectInfos);
    void dumpSnapshot(const CPlusPlus::Snapshot &snapshot, const QString &title,
                      bool isGlobalSnapshot = false);
    void dumpWorkingCopy(const CppModelManagerInterface::WorkingCopy &workingCopy);

private:
    void dumpDocuments(const QList<CPlusPlus::Document::Ptr> &documents,
                       bool skipDetails = false);
    static QByteArray indent(int level);

    CPlusPlus::Snapshot m_globalSnapshot;
    QFile m_logFile;
    QTextStream m_out;
};

CppCodeModelInspectorDumper::CppCodeModelInspectorDumper(const CPlusPlus::Snapshot &globalSnapshot)
    : m_globalSnapshot(globalSnapshot), m_out(stderr)
{
    const QString logFileName = QDir::tempPath()
        + QString::fromLatin1("/qtc-codemodelinspection.txt");
    m_logFile.setFileName(logFileName);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_out << "Code model inspection log file is \"" << QDir::toNativeSeparators(logFileName)
              << "\".\n";
        m_out.setDevice(&m_logFile);
    }
    m_out << "*** START Code Model Inspection Report for ";
    QString ideRevision;
#ifdef IDE_REVISION
     ideRevision = QLatin1String(" from revision ")
        + QString::fromLatin1(Core::Constants::IDE_REVISION_STR).left(10);
#endif
    m_out << Core::ICore::versionString() << ideRevision << "\n";
    m_out << "Note: This file contains vim fold markers (\"{{{n\"). "
             "Make use of them via \":set foldmethod=marker\".\n";
}

CppCodeModelInspectorDumper::~CppCodeModelInspectorDumper()
{
    m_out << "*** END Code Model Inspection Report\n";
}

void CppCodeModelInspectorDumper::dumpProjectInfos(
        const QList<CppModelManagerInterface::ProjectInfo> &projectInfos)
{
    const QByteArray i1 = indent(1);
    const QByteArray i2 = indent(2);
    const QByteArray i3 = indent(3);
    const QByteArray i4 = indent(4);

    m_out << "Projects loaded: " << projectInfos.size() << "{{{1\n";
    foreach (const CppModelManagerInterface::ProjectInfo &info, projectInfos) {
        const QPointer<ProjectExplorer::Project> project = info.project();
        m_out << i1 << "Project " << project->displayName() << " (" << project->projectFilePath()
              << "){{{2\n";

        const QList<ProjectPart::Ptr> projectParts = info.projectParts();
        foreach (const ProjectPart::Ptr &part, projectParts) {
            QString projectName = QLatin1String("<None>");
            QString projectFilePath = QLatin1String("<None>");
            if (ProjectExplorer::Project *project = part->project) {
                projectName = project->displayName();
                projectFilePath = project->projectFilePath();
            }
            if (!part->projectConfigFile.isEmpty())
                m_out << i3 << "Project Config File: " << part->projectConfigFile << "\n";
            m_out << i2 << "Project Part \"" << part->projectFile << "\"{{{3\n";
            m_out << i3 << "Project Part Name  : " << part->displayName << "\n";
            m_out << i3 << "Project Name       : " << projectName << "\n";
            m_out << i3 << "Project File       : " << projectFilePath << "\n";
            m_out << i3 << "C Version          : " << toString(part->cVersion) << "\n";
            m_out << i3 << "CXX Version        : " << toString(part->cxxVersion) << "\n";
            m_out << i3 << "CXX Extensions     : " << toString(part->cxxExtensions) << "\n";
            m_out << i3 << "Qt Version         : " << toString(part->qtVersion) << "\n";

            if (!part->files.isEmpty()) {
                m_out << i3 << "Files:{{{4\n";
                foreach (const ProjectFile &projectFile, part->files) {
                    m_out << i4 << toString(projectFile.kind) << ": " << projectFile.path
                          << "\n";
                }
            }

            if (!part->toolchainDefines.isEmpty()) {
                m_out << i3 << "Toolchain Defines:{{{4\n";
                const QList<QByteArray> defineLines = part->toolchainDefines.split('\n');
                foreach (const QByteArray &defineLine, defineLines)
                    m_out << i4 << defineLine << "\n";
            }
            if (!part->projectDefines.isEmpty()) {
                m_out << i3 << "Project Defines:{{{4\n";
                const QList<QByteArray> defineLines = part->projectDefines.split('\n');
                foreach (const QByteArray &defineLine, defineLines)
                    m_out << i4 << defineLine << "\n";
            }

            if (!part->includePaths.isEmpty()) {
                m_out << i3 << "Include Paths:{{{4\n";
                foreach (const QString &includePath, part->includePaths)
                    m_out << i4 << includePath << "\n";
            }

            if (!part->frameworkPaths.isEmpty()) {
                m_out << i3 << "Framework Paths:{{{4\n";
                foreach (const QString &frameworkPath, part->frameworkPaths)
                    m_out << i4 << frameworkPath << "\n";
            }

            if (!part->precompiledHeaders.isEmpty()) {
                m_out << i3 << "Precompiled Headers:{{{4\n";
                foreach (const QString &precompiledHeader, part->precompiledHeaders)
                    m_out << i4 << precompiledHeader << "\n";
            }
        } // for part
    } // for project Info
}

void CppCodeModelInspectorDumper::dumpSnapshot(const CPlusPlus::Snapshot &snapshot,
                                               const QString &title, bool isGlobalSnapshot)
{
    m_out << "Snapshot \"" << title << "\"{{{1\n";

    const QByteArray i1 = indent(1);
    const QList<CPlusPlus::Document::Ptr> documents = snapshotToList(snapshot);

    if (isGlobalSnapshot) {
        if (!documents.isEmpty()) {
            m_out << i1 << "Globally-Shared documents{{{2\n";
            dumpDocuments(documents, false);
        }
    } else {
        // Divide into shared and not shared
        QList<CPlusPlus::Document::Ptr> globallyShared;
        QList<CPlusPlus::Document::Ptr> notGloballyShared;
        foreach (const CPlusPlus::Document::Ptr &document, documents) {
            CPlusPlus::Document::Ptr globalDocument = m_globalSnapshot.document(document->fileName());
            if (globalDocument && globalDocument->fingerprint() == document->fingerprint())
                globallyShared.append(document);
            else
                notGloballyShared.append(document);
        }

        if (!notGloballyShared.isEmpty()) {
            m_out << i1 << "Not-Globally-Shared documents:{{{2\n";
            dumpDocuments(notGloballyShared);
        }
        if (!globallyShared.isEmpty()) {
            m_out << i1 << "Globally-Shared documents{{{2\n";
            dumpDocuments(globallyShared, true);
        }
    }
}

void CppCodeModelInspectorDumper::dumpWorkingCopy(
        const CppModelManagerInterface::WorkingCopy &workingCopy)
{
    m_out << "Working Copy contains " << workingCopy.size() << " entries{{{1\n";

    const QByteArray i1 = indent(1);
    QHashIterator<QString, QPair<QByteArray, unsigned> > it = workingCopy.iterator();
    while (it.hasNext()) {
        it.next();
        const QString filePath = it.key();
        unsigned sourcRevision = it.value().second;
        m_out << i1 << "rev=" << sourcRevision << ", " << filePath << "\n";
    }
}

void CppCodeModelInspectorDumper::dumpDocuments(const QList<CPlusPlus::Document::Ptr> &documents,
                                                bool skipDetails)
{
    const QByteArray i2 = indent(2);
    const QByteArray i3 = indent(3);
    const QByteArray i4 = indent(4);
    foreach (const CPlusPlus::Document::Ptr &document, documents) {
        if (skipDetails) {
            m_out << i2 << "\"" << document->fileName() << "\"\n";
            continue;
        }

        m_out << i2 << "Document \"" << document->fileName() << "\"{{{3\n";
        m_out << i3 << "Last Modified  : " << toString(document->lastModified()) << "\n";
        m_out << i3 << "Revision       : " << toString(document->revision()) << "\n";
        m_out << i3 << "Editor Revision: " << toString(document->editorRevision()) << "\n";
        m_out << i3 << "Check Mode     : " << toString(document->checkMode()) << "\n";
        m_out << i3 << "Tokenized      : " << toString(document->isTokenized()) << "\n";
        m_out << i3 << "Parsed         : " << toString(document->isParsed()) << "\n";
        m_out << i3 << "Project Parts  : " << partsForFile(document->fileName()) << "\n";

        const QList<CPlusPlus::Document::Include> includes = document->unresolvedIncludes()
                + document->resolvedIncludes();
        if (!includes.isEmpty()) {
            m_out << i3 << "Includes:{{{4\n";
            foreach (const CPlusPlus::Document::Include &include, includes) {
                m_out << i4 << "at line " << include.line() << ": "
                      << unresolvedFileNameWithDelimiters(include) << " ==> "
                      << include.resolvedFileName() << "\n";
            }
        }

        const QList<CPlusPlus::Document::DiagnosticMessage> diagnosticMessages
                = document->diagnosticMessages();
        if (!diagnosticMessages.isEmpty()) {
            m_out << i3 << "Diagnostic Messages:{{{4\n";
            foreach (const CPlusPlus::Document::DiagnosticMessage &msg, diagnosticMessages) {
                const CPlusPlus::Document::DiagnosticMessage::Level level
                        = static_cast<CPlusPlus::Document::DiagnosticMessage::Level>(msg.level());
                m_out << i4 << "at " << msg.line() << ":" << msg.column() << ", " << toString(level)
                      << ": " << msg.text() << "\n";
            }
        }

        const QList<CPlusPlus::Macro> macroDefinitions = document->definedMacros();
        if (!macroDefinitions.isEmpty()) {
            m_out << i3 << "(Un)Defined Macros:{{{4\n";
            foreach (const CPlusPlus::Macro &macro, macroDefinitions)
                m_out << i4 << "at line " << macro.line() << ": " << macro.toString() << "\n";
        }

        const QList<CPlusPlus::Document::MacroUse> macroUses = document->macroUses();
        if (!macroUses.isEmpty()) {
            m_out << i3 << "Macro Uses:{{{4\n";
            foreach (const CPlusPlus::Document::MacroUse &use, macroUses) {
                const QString type = use.isFunctionLike()
                        ? QLatin1String("function-like") : QLatin1String("object-like");
                m_out << i4 << "at line " << use.beginLine() << ", "
                      << QString::fromUtf8(use.macro().name()) << ", begin=" << use.begin()
                      << ", end=" << use.end() << ", " << type << ", args="
                      << use.arguments().size() << "\n";
            }
        }

        const QString source = QString::fromUtf8(document->utf8Source());
        if (!source.isEmpty()) {
            m_out << i4 << "Source:{{{4\n";
            m_out << source;
            m_out << "\n<<<EOF\n";
        }
    }
}

QByteArray CppCodeModelInspectorDumper::indent(int level)
{
    const QByteArray basicIndent("  ");
    QByteArray indent = basicIndent;
    while (level-- > 1)
        indent += basicIndent;
    return indent;
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

// --- FilterableView -----------------------------------------------------------------------------

class FilterableView : public QWidget
{
    Q_OBJECT
public:
    FilterableView(QWidget *parent);

    void setModel(QAbstractItemModel *model);
    QItemSelectionModel *selectionModel() const;
    void selectIndex(const QModelIndex &index);
    void resizeColumns(int columnCount);

signals:
    void filterChanged(const QString &filterText);

public slots:
    void clearFilter();

private:
    QTreeView *view;
    QLineEdit *lineEdit;
};

FilterableView::FilterableView(QWidget *parent)
    : QWidget(parent)
{
    view = new QTreeView(this);
    view->setAlternatingRowColors(true);
    view->setTextElideMode(Qt::ElideMiddle);
    view->setSortingEnabled(true);

    lineEdit = new QLineEdit(this);
    lineEdit->setPlaceholderText(QLatin1String("File Path"));
    QObject::connect(lineEdit, SIGNAL(textChanged(QString)), SIGNAL(filterChanged(QString)));

    QLabel *label = new QLabel(QLatin1String("&Filter:"), this);
    label->setBuddy(lineEdit);

    QPushButton *clearButton = new QPushButton(QLatin1String("&Clear"), this);
    QObject::connect(clearButton, SIGNAL(clicked()), SLOT(clearFilter()));

    QHBoxLayout *filterBarLayout = new QHBoxLayout();
    filterBarLayout->addWidget(label);
    filterBarLayout->addWidget(lineEdit);
    filterBarLayout->addWidget(clearButton);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(view);
    mainLayout->addLayout(filterBarLayout);

    setLayout(mainLayout);
}

void FilterableView::setModel(QAbstractItemModel *model)
{
    view->setModel(model);
}

QItemSelectionModel *FilterableView::selectionModel() const
{
    return view->selectionModel();
}

void FilterableView::selectIndex(const QModelIndex &index)
{
    if (index.isValid())  {
        view->selectionModel()->setCurrentIndex(index,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
}

void FilterableView::resizeColumns(int columnCount)
{
    for (int column = 0; column < columnCount - 1; ++column)
        view->resizeColumnToContents(column);
}

void FilterableView::clearFilter()
{
    lineEdit->clear();
}

// --- KeyValueModel ------------------------------------------------------------------------------

class KeyValueModel : public QAbstractListModel
{
    Q_OBJECT
public:
    typedef QList<QPair<QString, QString> > Table;

    KeyValueModel(QObject *parent);
    void configure(const Table &table);
    void clear();

    enum Columns { KeyColumn, ValueColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    Table m_table;
};

KeyValueModel::KeyValueModel(QObject *parent) : QAbstractListModel(parent)
{
}

void KeyValueModel::configure(const Table &table)
{
    emit layoutAboutToBeChanged();
    m_table = table;
    emit layoutChanged();
}

void KeyValueModel::clear()
{
    emit layoutAboutToBeChanged();
    m_table.clear();
    emit layoutChanged();
}

int KeyValueModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_table.size();
}

int KeyValueModel::columnCount(const QModelIndex &/*parent*/) const
{
    return KeyValueModel::ColumnCount;
}

QVariant KeyValueModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int row = index.row();
        const int column = index.column();
        if (column == KeyColumn) {
            return m_table.at(row).first;
        } else if (column == ValueColumn) {
            return m_table.at(row).second;
        }
    }
    return QVariant();
}

QVariant KeyValueModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case KeyColumn:
            return QLatin1String("Key");
        case ValueColumn:
            return QLatin1String("Value");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- SnapshotModel ------------------------------------------------------------------------------

class SnapshotModel : public QAbstractListModel
{
    Q_OBJECT
public:
    SnapshotModel(QObject *parent);
    void configure(const CPlusPlus::Snapshot &snapshot);
    void setGlobalSnapshot(const CPlusPlus::Snapshot &snapshot);

    QModelIndex indexForDocument(const QString &filePath);

    enum Columns { SymbolCountColumn, SharedColumn, FilePathColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<CPlusPlus::Document::Ptr> m_documents;
    CPlusPlus::Snapshot m_globalSnapshot;
};

SnapshotModel::SnapshotModel(QObject *parent) : QAbstractListModel(parent)
{
}

void SnapshotModel::configure(const CPlusPlus::Snapshot &snapshot)
{
    emit layoutAboutToBeChanged();
    m_documents = snapshotToList(snapshot);
    emit layoutChanged();
}

void SnapshotModel::setGlobalSnapshot(const CPlusPlus::Snapshot &snapshot)
{
    m_globalSnapshot = snapshot;
}

QModelIndex SnapshotModel::indexForDocument(const QString &filePath)
{
    for (int i = 0, total = m_documents.size(); i < total; ++i) {
        const CPlusPlus::Document::Ptr document = m_documents.at(i);
        if (document->fileName() == filePath)
            return index(i, FilePathColumn);
    }
    return QModelIndex();
}

int SnapshotModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_documents.size();
}

int SnapshotModel::columnCount(const QModelIndex &/*parent*/) const
{
    return SnapshotModel::ColumnCount;
}

QVariant SnapshotModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const int column = index.column();
        CPlusPlus::Document::Ptr document = m_documents.at(index.row());
        if (column == SymbolCountColumn) {
            return document->control()->symbolCount();
        } else if (column == SharedColumn) {
            CPlusPlus::Document::Ptr globalDocument = m_globalSnapshot.document(document->fileName());
            const bool isShared
                = globalDocument && globalDocument->fingerprint() == document->fingerprint();
            return toString(isShared);
        } else if (column == FilePathColumn) {
            return QDir::toNativeSeparators(document->fileName());
        }
    }
    return QVariant();
}

QVariant SnapshotModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case SymbolCountColumn:
            return QLatin1String("Symbols");
        case SharedColumn:
            return QLatin1String("Shared");
        case FilePathColumn:
            return QLatin1String("File Path");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- IncludesModel ------------------------------------------------------------------------------

static bool includesSorter(const CPlusPlus::Document::Include &i1,
                           const CPlusPlus::Document::Include &i2)
{
    return i1.line() < i2.line();
}

class IncludesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    IncludesModel(QObject *parent);
    void configure(const QList<CPlusPlus::Document::Include> &includes);
    void clear();

    enum Columns { ResolvedOrNotColumn, LineNumberColumn, FilePathsColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<CPlusPlus::Document::Include> m_includes;
};

IncludesModel::IncludesModel(QObject *parent) : QAbstractListModel(parent)
{
}

void IncludesModel::configure(const QList<CPlusPlus::Document::Include> &includes)
{
    emit layoutAboutToBeChanged();
    m_includes = includes;
    qStableSort(m_includes.begin(), m_includes.end(), includesSorter);
    emit layoutChanged();
}

void IncludesModel::clear()
{
    emit layoutAboutToBeChanged();
    m_includes.clear();
    emit layoutChanged();
}

int IncludesModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_includes.size();
}

int IncludesModel::columnCount(const QModelIndex &/*parent*/) const
{
    return IncludesModel::ColumnCount;
}

QVariant IncludesModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::ForegroundRole)
        return QVariant();

    static const QBrush greenBrush(QColor(0, 139, 69));
    static const QBrush redBrush(QColor(205, 38, 38));

    const CPlusPlus::Document::Include include = m_includes.at(index.row());
    const QString resolvedFileName = QDir::toNativeSeparators(include.resolvedFileName());
    const bool isResolved = !resolvedFileName.isEmpty();

    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == ResolvedOrNotColumn) {
            return toString(isResolved);
        } else if (column == LineNumberColumn) {
            return include.line();
        } else if (column == FilePathsColumn) {
            return QVariant(unresolvedFileNameWithDelimiters(include) + QLatin1String(" --> ")
                            + resolvedFileName);
        }
    } else if (role == Qt::ForegroundRole) {
        return isResolved ? greenBrush : redBrush;
    }

    return QVariant();
}

QVariant IncludesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ResolvedOrNotColumn:
            return QLatin1String("Resolved");
        case LineNumberColumn:
            return QLatin1String("Line");
        case FilePathsColumn:
            return QLatin1String("File Paths");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- DiagnosticMessagesModel --------------------------------------------------------------------

static bool diagnosticMessagesModelSorter(const CPlusPlus::Document::DiagnosticMessage &m1,
                                          const CPlusPlus::Document::DiagnosticMessage &m2)
{
    return m1.line() < m2.line();
}

class DiagnosticMessagesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    DiagnosticMessagesModel(QObject *parent);
    void configure(const QList<CPlusPlus::Document::DiagnosticMessage> &messages);
    void clear();

    enum Columns { LevelColumn, LineColumnNumberColumn, MessageColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<CPlusPlus::Document::DiagnosticMessage> m_messages;
};

DiagnosticMessagesModel::DiagnosticMessagesModel(QObject *parent) : QAbstractListModel(parent)
{
}

void DiagnosticMessagesModel::configure(
        const QList<CPlusPlus::Document::DiagnosticMessage> &messages)
{
    emit layoutAboutToBeChanged();
    m_messages = messages;
    qStableSort(m_messages.begin(), m_messages.end(), diagnosticMessagesModelSorter);
    emit layoutChanged();
}

void DiagnosticMessagesModel::clear()
{
    emit layoutAboutToBeChanged();
    m_messages.clear();
    emit layoutChanged();
}

int DiagnosticMessagesModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_messages.size();
}

int DiagnosticMessagesModel::columnCount(const QModelIndex &/*parent*/) const
{
    return DiagnosticMessagesModel::ColumnCount;
}

QVariant DiagnosticMessagesModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::ForegroundRole)
        return QVariant();

    static const QBrush yellowOrangeBrush(QColor(237, 145, 33));
    static const QBrush redBrush(QColor(205, 38, 38));
    static const QBrush darkRedBrushQColor(QColor(139, 0, 0));

    const CPlusPlus::Document::DiagnosticMessage message = m_messages.at(index.row());
    const CPlusPlus::Document::DiagnosticMessage::Level level
        = static_cast<CPlusPlus::Document::DiagnosticMessage::Level>(message.level());

    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == LevelColumn) {
            return toString(level);
        } else if (column == LineColumnNumberColumn) {
            return QVariant(QString::number(message.line()) + QLatin1Char(':')
                            + QString::number(message.column()));
        } else if (column == MessageColumn) {
            return message.text();
        }
    } else if (role == Qt::ForegroundRole) {
        switch (level) {
        case CPlusPlus::Document::DiagnosticMessage::Warning:
            return yellowOrangeBrush;
        case CPlusPlus::Document::DiagnosticMessage::Error:
            return redBrush;
        case CPlusPlus::Document::DiagnosticMessage::Fatal:
            return darkRedBrushQColor;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

QVariant DiagnosticMessagesModel::headerData(int section, Qt::Orientation orientation, int role)
    const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case LevelColumn:
            return QLatin1String("Level");
        case LineColumnNumberColumn:
            return QLatin1String("Line:Column");
        case MessageColumn:
            return QLatin1String("Message");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- MacrosModel --------------------------------------------------------------------------------

class MacrosModel : public QAbstractListModel
{
    Q_OBJECT
public:
    MacrosModel(QObject *parent);
    void configure(const QList<CPlusPlus::Macro> &macros);
    void clear();

    enum Columns { LineNumberColumn, MacroColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<CPlusPlus::Macro> m_macros;
};

MacrosModel::MacrosModel(QObject *parent) : QAbstractListModel(parent)
{
}

void MacrosModel::configure(const QList<CPlusPlus::Macro> &macros)
{
    emit layoutAboutToBeChanged();
    m_macros = macros;
    emit layoutChanged();
}

void MacrosModel::clear()
{
    emit layoutAboutToBeChanged();
    m_macros.clear();
    emit layoutChanged();
}

int MacrosModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_macros.size();
}

int MacrosModel::columnCount(const QModelIndex &/*parent*/) const
{
    return MacrosModel::ColumnCount;
}

QVariant MacrosModel::data(const QModelIndex &index, int role) const
{
    const int column = index.column();
    if (role == Qt::DisplayRole || (role == Qt::ToolTipRole && column == MacroColumn)) {
        const CPlusPlus::Macro macro = m_macros.at(index.row());
        if (column == LineNumberColumn)
            return macro.line();
        else if (column == MacroColumn)
            return macro.toString();
    } else if (role == Qt::TextAlignmentRole) {
        return Qt::AlignTop + Qt::AlignLeft;
    }
    return QVariant();
}

QVariant MacrosModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case LineNumberColumn:
            return QLatin1String("Line");
        case MacroColumn:
            return QLatin1String("Macro");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- SymbolsModel -------------------------------------------------------------------------------

class SymbolsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    SymbolsModel(QObject *parent);
    void configure(const CPlusPlus::Document::Ptr &document);
    void clear();

    enum Columns { SymbolColumn, LineNumberColumn, ColumnCount };

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    CPlusPlus::Document::Ptr m_document;
};

SymbolsModel::SymbolsModel(QObject *parent) : QAbstractItemModel(parent)
{
}

void SymbolsModel::configure(const CPlusPlus::Document::Ptr &document)
{
    QTC_CHECK(document);
    emit layoutAboutToBeChanged();
    m_document = document;
    emit layoutChanged();
}

void SymbolsModel::clear()
{
    emit layoutAboutToBeChanged();
    m_document.clear();
    emit layoutChanged();
}

static CPlusPlus::Symbol *indexToSymbol(const QModelIndex &index)
{
    if (CPlusPlus::Symbol *symbol = static_cast<CPlusPlus::Symbol*>(index.internalPointer()))
        return symbol;
    return 0;
}

static CPlusPlus::Scope *indexToScope(const QModelIndex &index)
{
    if (CPlusPlus::Symbol *symbol = indexToSymbol(index))
        return symbol->asScope();
    return 0;
}

QModelIndex SymbolsModel::index(int row, int column, const QModelIndex &parent) const
{
    CPlusPlus::Scope *scope = 0;
    if (parent.isValid())
        scope = indexToScope(parent);
    else if (m_document)
        scope = m_document->globalNamespace();

    if (scope) {
        if ((unsigned)row < scope->memberCount())
            return createIndex(row, column, scope->memberAt(row));
    }

    return QModelIndex();
}

QModelIndex SymbolsModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    if (CPlusPlus::Symbol *symbol = indexToSymbol(child)) {
        if (CPlusPlus::Scope *scope = symbol->enclosingScope()) {
            const int row = DepthFinder()(m_document, scope);
            return createIndex(row, 0, scope);
        }
    }

    return QModelIndex();
}

int SymbolsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (CPlusPlus::Scope *scope = indexToScope(parent))
            return scope->memberCount();
    } else {
        if (m_document)
            return m_document->globalNamespace()->memberCount();
    }
    return 0;
}

int SymbolsModel::columnCount(const QModelIndex &) const
{
    return ColumnCount;
}

QVariant SymbolsModel::data(const QModelIndex &index, int role) const
{
    const int column = index.column();
    if (role == Qt::DisplayRole) {
        CPlusPlus::Symbol *symbol = indexToSymbol(index);
        if (!symbol)
            return QVariant();
        if (column == LineNumberColumn) {
            return symbol->line();
        } else if (column == SymbolColumn) {
            QString name = CPlusPlus::Overview().prettyName(symbol->name());
            if (name.isEmpty())
                name = QLatin1String("<no name>");
            return name;
        }
    }
    return QVariant();
}

QVariant SymbolsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case SymbolColumn:
            return QLatin1String("Symbol");
        case LineNumberColumn:
            return QLatin1String("Line");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- TokensModel --------------------------------------------------------------------------------

class TokensModel : public QAbstractListModel
{
    Q_OBJECT
public:
    TokensModel(QObject *parent);
    void configure(CPlusPlus::TranslationUnit *translationUnit);
    void clear();

    enum Columns { SpelledColumn, KindColumn, IndexColumn, OffsetColumn, LineColumnNumberColumn,
                   LengthColumn, GeneratedColumn, ExpandedColumn, WhiteSpaceColumn, NewlineColumn,
                   ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    struct TokenInfo {
        CPlusPlus::Token token;
        unsigned line;
        unsigned column;
    };
    QList<TokenInfo> m_tokenInfos;
};

TokensModel::TokensModel(QObject *parent) : QAbstractListModel(parent)
{
}

void TokensModel::configure(CPlusPlus::TranslationUnit *translationUnit)
{
    if (!translationUnit)
        return;

    emit layoutAboutToBeChanged();
    m_tokenInfos.clear();
    for (int i = 0, total = translationUnit->tokenCount(); i < total; ++i) {
        TokenInfo info;
        info.token = translationUnit->tokenAt(i);
        translationUnit->getPosition(info.token.offset, &info.line, &info.column);
        m_tokenInfos.append(info);
    }
    emit layoutChanged();
}

void TokensModel::clear()
{
    emit layoutAboutToBeChanged();
    m_tokenInfos.clear();
    emit layoutChanged();
}

int TokensModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_tokenInfos.size();
}

int TokensModel::columnCount(const QModelIndex &/*parent*/) const
{
    return TokensModel::ColumnCount;
}

QVariant TokensModel::data(const QModelIndex &index, int role) const
{
    const int column = index.column();
    if (role == Qt::DisplayRole) {
        const TokenInfo info = m_tokenInfos.at(index.row());
        const CPlusPlus::Token token = info.token;
        if (column == SpelledColumn)
            return QString::fromUtf8(token.spell());
        else if (column == KindColumn)
            return toString(static_cast<CPlusPlus::Kind>(token.kind()));
        else if (column == IndexColumn)
            return index.row();
        else if (column == OffsetColumn)
            return token.offset;
        else if (column == LineColumnNumberColumn)
            return QString::fromLatin1("%1:%2").arg(toString(info.line), toString(info.column));
        else if (column == LengthColumn)
            return toString(token.length());
        else if (column == GeneratedColumn)
            return toString(token.generated());
        else if (column == ExpandedColumn)
            return toString(token.expanded());
        else if (column == WhiteSpaceColumn)
            return toString(token.whitespace());
        else if (column == NewlineColumn)
            return toString(token.newline());
    } else if (role == Qt::TextAlignmentRole) {
        return Qt::AlignTop + Qt::AlignLeft;
    }
    return QVariant();
}

QVariant TokensModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case SpelledColumn:
            return QLatin1String("Spelled");
        case KindColumn:
            return QLatin1String("Kind");
        case IndexColumn:
            return QLatin1String("Index");
        case OffsetColumn:
            return QLatin1String("Offset");
        case LineColumnNumberColumn:
            return QLatin1String("Line:Column");
        case LengthColumn:
            return QLatin1String("Length");
        case GeneratedColumn:
            return QLatin1String("Generated");
        case ExpandedColumn:
            return QLatin1String("Expanded");
        case WhiteSpaceColumn:
            return QLatin1String("Whitespace");
        case NewlineColumn:
            return QLatin1String("Newline");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- ProjectPartsModel --------------------------------------------------------------------------

class ProjectPartsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    ProjectPartsModel(QObject *parent);

    void configure(const QList<CppModelManagerInterface::ProjectInfo> &projectInfos,
                   const ProjectPart::Ptr &currentEditorsProjectPart);

    QModelIndex indexForCurrentEditorsProjectPart() const;
    ProjectPart::Ptr projectPartForProjectFile(const QString &projectFilePath) const;

    enum Columns { PartNameColumn, PartFilePathColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    QList<ProjectPart::Ptr> m_projectPartsList;
    int m_currentEditorsProjectPartIndex;
};

ProjectPartsModel::ProjectPartsModel(QObject *parent)
    : QAbstractListModel(parent), m_currentEditorsProjectPartIndex(-1)
{
}

void ProjectPartsModel::configure(const QList<CppModelManagerInterface::ProjectInfo> &projectInfos,
                                  const ProjectPart::Ptr &currentEditorsProjectPart)
{
    emit layoutAboutToBeChanged();
    m_projectPartsList.clear();
    foreach (const CppModelManagerInterface::ProjectInfo &info, projectInfos) {
        foreach (const ProjectPart::Ptr &projectPart, info.projectParts()) {
            if (!m_projectPartsList.contains(projectPart)) {
                m_projectPartsList << projectPart;
                if (projectPart == currentEditorsProjectPart)
                    m_currentEditorsProjectPartIndex = m_projectPartsList.size() - 1;
            }
        }
    }
    emit layoutChanged();
}

QModelIndex ProjectPartsModel::indexForCurrentEditorsProjectPart() const
{
    if (m_currentEditorsProjectPartIndex == -1)
        return QModelIndex();
    return createIndex(m_currentEditorsProjectPartIndex, PartFilePathColumn);
}

ProjectPart::Ptr ProjectPartsModel::projectPartForProjectFile(const QString &projectFilePath) const
{
    foreach (const ProjectPart::Ptr &part, m_projectPartsList) {
        if (part->projectFile == projectFilePath)
            return part;
    }
    return ProjectPart::Ptr();
}

int ProjectPartsModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_projectPartsList.size();
}

int ProjectPartsModel::columnCount(const QModelIndex &/*parent*/) const
{
    return ProjectPartsModel::ColumnCount;
}

QVariant ProjectPartsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == PartNameColumn)
            return m_projectPartsList.at(row)->displayName;
        else if (column == PartFilePathColumn)
            return QDir::toNativeSeparators(m_projectPartsList.at(row)->projectFile);
    }
    return QVariant();
}

QVariant ProjectPartsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case PartNameColumn:
            return QLatin1String("Name");
        case PartFilePathColumn:
            return QLatin1String("Project File Path");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- WorkingCopyModel ---------------------------------------------------------------------------

class WorkingCopyModel : public QAbstractListModel
{
    Q_OBJECT
public:
    WorkingCopyModel(QObject *parent);

    void configure(const CppModelManagerInterface::WorkingCopy &workingCopy);
    QModelIndex indexForFile(const QString &filePath);

    enum Columns { RevisionColumn, FilePathColumn, ColumnCount };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private:
    struct WorkingCopyEntry {
        WorkingCopyEntry(const QString &filePath, const QByteArray &source, unsigned revision)
            : filePath(filePath), source(source), revision(revision)
        {}

        QString filePath;
        QByteArray source;
        unsigned revision;
    };

    QList<WorkingCopyEntry> m_workingCopyList;
};

WorkingCopyModel::WorkingCopyModel(QObject *parent) : QAbstractListModel(parent)
{
}

void WorkingCopyModel::configure(const CppModelManagerInterface::WorkingCopy &workingCopy)
{
    emit layoutAboutToBeChanged();
    m_workingCopyList.clear();
    QHashIterator<QString, QPair<QByteArray, unsigned> > it = workingCopy.iterator();
    while (it.hasNext()) {
        it.next();
        m_workingCopyList << WorkingCopyEntry(it.key(), it.value().first, it.value().second);
    }
    emit layoutChanged();
}

QModelIndex WorkingCopyModel::indexForFile(const QString &filePath)
{
    for (int i = 0, total = m_workingCopyList.size(); i < total; ++i) {
        const WorkingCopyEntry entry = m_workingCopyList.at(i);
        if (entry.filePath == filePath)
            return index(i, FilePathColumn);
    }
    return QModelIndex();
}

int WorkingCopyModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_workingCopyList.size();
}

int WorkingCopyModel::columnCount(const QModelIndex &/*parent*/) const
{
    return WorkingCopyModel::ColumnCount;
}

QVariant WorkingCopyModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (role == Qt::DisplayRole) {
        const int column = index.column();
        if (column == RevisionColumn)
            return m_workingCopyList.at(row).revision;
        else if (column == FilePathColumn)
            return m_workingCopyList.at(row).filePath;
    } else if (role == Qt::UserRole) {
        return m_workingCopyList.at(row).source;
    }
    return QVariant();
}

QVariant WorkingCopyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case RevisionColumn:
            return QLatin1String("Revision");
        case FilePathColumn:
            return QLatin1String("File Path");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

// --- SnapshotInfo -------------------------------------------------------------------------------

class SnapshotInfo
{
public:
    enum Type { GlobalSnapshot, EditorSnapshot };
    SnapshotInfo(const CPlusPlus::Snapshot &snapshot, Type type)
        : snapshot(snapshot), type(type) {}

    CPlusPlus::Snapshot snapshot;
    Type type;
};

// --- CppCodeModelInspectorDialog ----------------------------------------------------------------

CppCodeModelInspectorDialog::CppCodeModelInspectorDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::CppCodeModelInspectorDialog)
    , m_snapshotInfos(new QList<SnapshotInfo>())
    , m_snapshotView(new FilterableView(this))
    , m_snapshotModel(new SnapshotModel(this))
    , m_proxySnapshotModel(new QSortFilterProxyModel(this))
    , m_docGenericInfoModel(new KeyValueModel(this))
    , m_docIncludesModel(new IncludesModel(this))
    , m_docDiagnosticMessagesModel(new DiagnosticMessagesModel(this))
    , m_docMacrosModel(new MacrosModel(this))
    , m_docSymbolsModel(new SymbolsModel(this))
    , m_docTokensModel(new TokensModel(this))
    , m_projectPartsView(new FilterableView(this))
    , m_projectPartsModel(new ProjectPartsModel(this))
    , m_proxyProjectPartsModel(new QSortFilterProxyModel(this))
    , m_partGenericInfoModel(new KeyValueModel(this))
    , m_workingCopyView(new FilterableView(this))
    , m_workingCopyModel(new WorkingCopyModel(this))
    , m_proxyWorkingCopyModel(new QSortFilterProxyModel(this))
{
    m_ui->setupUi(this);
    m_ui->snapshotSelectorAndViewLayout->addWidget(m_snapshotView);
    m_ui->projectPartsSplitter->insertWidget(0, m_projectPartsView);
    m_ui->workingCopySplitter->insertWidget(0, m_workingCopyView);

    setAttribute(Qt::WA_DeleteOnClose);
    connect(Core::ICore::instance(), SIGNAL(coreAboutToClose()), SLOT(close()));

    m_proxySnapshotModel->setSourceModel(m_snapshotModel);
    m_proxySnapshotModel->setFilterKeyColumn(SnapshotModel::FilePathColumn);
    m_snapshotView->setModel(m_proxySnapshotModel);
    m_ui->docGeneralView->setModel(m_docGenericInfoModel);
    m_ui->docIncludesView->setModel(m_docIncludesModel);
    m_ui->docDiagnosticMessagesView->setModel(m_docDiagnosticMessagesModel);
    m_ui->docDefinedMacrosView->setModel(m_docMacrosModel);
    m_ui->docSymbolsView->setModel(m_docSymbolsModel);
    m_ui->docTokensView->setModel(m_docTokensModel);

    m_proxyProjectPartsModel->setSourceModel(m_projectPartsModel);
    m_proxyProjectPartsModel->setFilterKeyColumn(ProjectPartsModel::PartFilePathColumn);
    m_projectPartsView->setModel(m_proxyProjectPartsModel);
    m_ui->partGeneralView->setModel(m_partGenericInfoModel);

    m_proxyWorkingCopyModel->setSourceModel(m_workingCopyModel);
    m_proxyWorkingCopyModel->setFilterKeyColumn(WorkingCopyModel::FilePathColumn);
    m_workingCopyView->setModel(m_proxyWorkingCopyModel);

    connect(m_snapshotView->selectionModel(),
            SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(onDocumentSelected(QModelIndex,QModelIndex)));
    connect(m_snapshotView, SIGNAL(filterChanged(QString)),
            SLOT(onSnapshotFilterChanged(QString)));
    connect(m_ui->snapshotSelector, SIGNAL(currentIndexChanged(int)),
            SLOT(onSnapshotSelected(int)));
    connect(m_ui->docSymbolsView, SIGNAL(expanded(QModelIndex)),
            SLOT(onSymbolsViewExpandedOrCollapsed(QModelIndex)));
    connect(m_ui->docSymbolsView, SIGNAL(collapsed(QModelIndex)),
            SLOT(onSymbolsViewExpandedOrCollapsed(QModelIndex)));

    connect(m_projectPartsView->selectionModel(),
            SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(onProjectPartSelected(QModelIndex,QModelIndex)));
    connect(m_projectPartsView, SIGNAL(filterChanged(QString)),
            SLOT(onProjectPartFilterChanged(QString)));

    connect(m_workingCopyView->selectionModel(),
            SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(onWorkingCopyDocumentSelected(QModelIndex,QModelIndex)));
    connect(m_workingCopyView, SIGNAL(filterChanged(QString)),
            SLOT(onWorkingCopyFilterChanged(QString)));

    connect(m_ui->refreshButton, SIGNAL(clicked()), SLOT(onRefreshRequested()));
    connect(m_ui->closeButton, SIGNAL(clicked()), SLOT(close()));

    refresh();
}

CppCodeModelInspectorDialog::~CppCodeModelInspectorDialog()
{
    delete m_snapshotInfos;
    delete m_ui;
}

void CppCodeModelInspectorDialog::onRefreshRequested()
{
    refresh();
}

void CppCodeModelInspectorDialog::onSnapshotFilterChanged(const QString &pattern)
{
    m_proxySnapshotModel->setFilterWildcard(pattern);
}

void CppCodeModelInspectorDialog::onSnapshotSelected(int row)
{
    if (row < 0 || row >= m_snapshotInfos->size())
        return;

    m_snapshotView->clearFilter();
    const SnapshotInfo info = m_snapshotInfos->at(row);
    m_snapshotModel->configure(info.snapshot);
    m_snapshotView->resizeColumns(SnapshotModel::ColumnCount);

    if (info.type == SnapshotInfo::GlobalSnapshot) {
        // Select first document
        const QModelIndex index = m_proxySnapshotModel->index(0, SnapshotModel::FilePathColumn);
        m_snapshotView->selectIndex(index);
    } else if (info.type == SnapshotInfo::EditorSnapshot) {
        // Select first document, unless we can find the editor document
        QModelIndex index = m_snapshotModel->indexForDocument(fileInCurrentEditor());
        index = m_proxySnapshotModel->mapFromSource(index);
        if (!index.isValid())
            index = m_proxySnapshotModel->index(0, SnapshotModel::FilePathColumn);
        m_snapshotView->selectIndex(index);
    }
}

void CppCodeModelInspectorDialog::onDocumentSelected(const QModelIndex &current,
                                                     const QModelIndex &)
{
    if (current.isValid()) {
        const QModelIndex index = m_proxySnapshotModel->index(current.row(),
                                                              SnapshotModel::FilePathColumn);
        const QString filePath = QDir::fromNativeSeparators(
            m_proxySnapshotModel->data(index, Qt::DisplayRole).toString());
        const SnapshotInfo info = m_snapshotInfos->at(m_ui->snapshotSelector->currentIndex());
        updateDocumentData(info.snapshot.document(filePath));
    } else {
        clearDocumentData();
    }
}

void CppCodeModelInspectorDialog::onSymbolsViewExpandedOrCollapsed(const QModelIndex &)
{
    resizeColumns<SymbolsModel>(m_ui->docSymbolsView);
}

void CppCodeModelInspectorDialog::onProjectPartFilterChanged(const QString &pattern)
{
    m_proxyProjectPartsModel->setFilterWildcard(pattern);
}

void CppCodeModelInspectorDialog::onProjectPartSelected(const QModelIndex &current,
                                                        const QModelIndex &)
{
    if (current.isValid()) {
        QModelIndex index = m_proxyProjectPartsModel->mapToSource(current);
        if (index.isValid()) {
            index = m_projectPartsModel->index(index.row(), ProjectPartsModel::PartFilePathColumn);
            const QString projectFilePath = QDir::fromNativeSeparators(
                m_projectPartsModel->data(index, Qt::DisplayRole).toString());
            updateProjectPartData(m_projectPartsModel->projectPartForProjectFile(projectFilePath));
        }
    } else {
        clearProjectPartData();
    }
}

void CppCodeModelInspectorDialog::onWorkingCopyFilterChanged(const QString &pattern)
{
    m_proxyWorkingCopyModel->setFilterWildcard(pattern);
}

void CppCodeModelInspectorDialog::onWorkingCopyDocumentSelected(const QModelIndex &current,
                                                                const QModelIndex &)
{
    if (current.isValid()) {
        const QModelIndex index = m_proxyWorkingCopyModel->mapToSource(current);
        if (index.isValid()) {
            const QString source
                = QString::fromUtf8(m_workingCopyModel->data(index, Qt::UserRole).toByteArray());
            m_ui->workingCopySourceEdit->setPlainText(source);
        }
    } else {
        m_ui->workingCopySourceEdit->setPlainText(QString());
    }
}

void CppCodeModelInspectorDialog::refresh()
{
    CppModelManagerInterface *cmm = CppModelManagerInterface::instance();

    const int oldSnapshotIndex = m_ui->snapshotSelector->currentIndex();
    const bool selectEditorRelevant
        = m_ui->selectEditorRelevantEntriesAfterRefreshCheckBox->isChecked();

    // Snapshots and Documents
    m_snapshotInfos->clear();
    m_ui->snapshotSelector->clear();

    const CPlusPlus::Snapshot globalSnapshot = cmm->snapshot();
    CppCodeModelInspectorDumper dumper(globalSnapshot);
    m_snapshotModel->setGlobalSnapshot(globalSnapshot);

    m_snapshotInfos->append(SnapshotInfo(globalSnapshot, SnapshotInfo::GlobalSnapshot));
    const QString globalSnapshotTitle
        = QString::fromLatin1("Global/Indexing Snapshot (%1 Documents)").arg(globalSnapshot.size());
    m_ui->snapshotSelector->addItem(globalSnapshotTitle);
    dumper.dumpSnapshot(globalSnapshot, globalSnapshotTitle, /*isGlobalSnapshot=*/ true);

    TextEditor::BaseTextEditor *editor = currentEditor();
    CppEditorSupport *editorSupport = 0;
    if (editor) {
        editorSupport = cmm->cppEditorSupport(editor);
        if (editorSupport) {
            const CPlusPlus::Snapshot editorSnapshot = editorSupport->snapshotUpdater()->snapshot();
            m_snapshotInfos->append(SnapshotInfo(editorSnapshot, SnapshotInfo::EditorSnapshot));
            const QString editorSnapshotTitle
                = QString::fromLatin1("Current Editor's Snapshot (%1 Documents)")
                    .arg(editorSnapshot.size());
            dumper.dumpSnapshot(editorSnapshot, editorSnapshotTitle);
            m_ui->snapshotSelector->addItem(editorSnapshotTitle);
        }
        CppEditor::Internal::CPPEditorWidget *cppEditorWidget
            = qobject_cast<CppEditor::Internal::CPPEditorWidget *>(editor->editorWidget());
        if (cppEditorWidget) {
            SemanticInfo semanticInfo = cppEditorWidget->semanticInfo();
            CPlusPlus::Snapshot snapshot;

            // Add semantic info snapshot
            snapshot = semanticInfo.snapshot;
            m_snapshotInfos->append(SnapshotInfo(snapshot, SnapshotInfo::EditorSnapshot));
            m_ui->snapshotSelector->addItem(
                QString::fromLatin1("Current Editor's Semantic Info Snapshot (%1 Documents)")
                    .arg(snapshot.size()));

            // Add a pseudo snapshot containing only the semantic info document since this document
            // is not part of the semantic snapshot.
            snapshot = CPlusPlus::Snapshot();
            snapshot.insert(cppEditorWidget->semanticInfo().doc);
            m_snapshotInfos->append(SnapshotInfo(snapshot, SnapshotInfo::EditorSnapshot));
            const QString snapshotTitle
                = QString::fromLatin1("Current Editor's Pseudo Snapshot with Semantic Info Document (%1 Documents)")
                    .arg(snapshot.size());
            dumper.dumpSnapshot(snapshot, snapshotTitle);
            m_ui->snapshotSelector->addItem(snapshotTitle);
        }
    }

    int snapshotIndex = 0;
    if (selectEditorRelevant) {
        for (int i = 0, total = m_snapshotInfos->size(); i < total; ++i) {
            const SnapshotInfo info = m_snapshotInfos->at(i);
            if (info.type == SnapshotInfo::EditorSnapshot) {
                snapshotIndex = i;
                break;
            }
        }
    } else if (oldSnapshotIndex < m_snapshotInfos->size()) {
        snapshotIndex = oldSnapshotIndex;
    }
    m_ui->snapshotSelector->setCurrentIndex(snapshotIndex);
    onSnapshotSelected(snapshotIndex);

    // Project Parts
    const ProjectPart::Ptr editorsProjectPart = editorSupport
        ? editorSupport->snapshotUpdater()->currentProjectPart()
        : ProjectPart::Ptr();

    const QList<CppModelManagerInterface::ProjectInfo> projectInfos = cmm->projectInfos();
    dumper.dumpProjectInfos(projectInfos);
    m_projectPartsModel->configure(projectInfos, editorsProjectPart);
    m_projectPartsView->resizeColumns(ProjectPartsModel::ColumnCount);
    QModelIndex index = m_proxyProjectPartsModel->index(0, ProjectPartsModel::PartFilePathColumn);
    if (index.isValid()) {
        if (selectEditorRelevant && editorsProjectPart) {
            QModelIndex editorPartIndex = m_projectPartsModel->indexForCurrentEditorsProjectPart();
            editorPartIndex = m_proxyProjectPartsModel->mapFromSource(editorPartIndex);
            if (editorPartIndex.isValid())
                index = editorPartIndex;
        }
        m_projectPartsView->selectIndex(index);
    }

    // Working Copy
    const CppModelManagerInterface::WorkingCopy workingCopy = cmm->workingCopy();
    dumper.dumpWorkingCopy(workingCopy);
    m_workingCopyModel->configure(workingCopy);
    m_workingCopyView->resizeColumns(WorkingCopyModel::ColumnCount);
    if (workingCopy.size() > 0) {
        QModelIndex index = m_proxyWorkingCopyModel->index(0, WorkingCopyModel::FilePathColumn);
        if (selectEditorRelevant) {
            const QModelIndex eindex = m_workingCopyModel->indexForFile(fileInCurrentEditor());
            if (eindex.isValid())
                index = m_proxyWorkingCopyModel->mapFromSource(eindex);
        }
        m_workingCopyView->selectIndex(index);
    }
}

enum DocumentTabs {
    DocumentGeneralTab,
    DocumentIncludesTab,
    DocumentDiagnosticsTab,
    DocumentDefinedMacrosTab,
    DocumentPreprocessedSourceTab,
    DocumentSymbolsTab,
    DocumentTokensTab
};

static QString docTabName(int tabIndex, int numberOfEntries = -1)
{
    const char *names[] = {
        "&General",
        "&Includes",
        "&Diagnostic Messages",
        "(Un)Defined &Macros",
        "P&reprocessed Source",
        "&Symbols",
        "&Tokens"
    };
    QString result = QLatin1String(names[tabIndex]);
    if (numberOfEntries != -1)
        result += QString::fromLatin1(" (%1)").arg(numberOfEntries);
    return result;
}

void CppCodeModelInspectorDialog::clearDocumentData()
{
    m_docGenericInfoModel->clear();

    m_ui->docTab->setTabText(DocumentIncludesTab, docTabName(DocumentIncludesTab));
    m_docIncludesModel->clear();

    m_ui->docTab->setTabText(DocumentDiagnosticsTab, docTabName(DocumentDiagnosticsTab));
    m_docDiagnosticMessagesModel->clear();

    m_ui->docTab->setTabText(DocumentDefinedMacrosTab, docTabName(DocumentDefinedMacrosTab));
    m_docMacrosModel->clear();

    m_ui->docPreprocessedSourceEdit->setPlainText(QString());

    m_docSymbolsModel->clear();

    m_ui->docTab->setTabText(DocumentTokensTab, docTabName(DocumentTokensTab));
    m_docTokensModel->clear();
}

void CppCodeModelInspectorDialog::updateDocumentData(const CPlusPlus::Document::Ptr &document)
{
    QTC_ASSERT(document, return);

    // General
    KeyValueModel::Table table = KeyValueModel::Table()
        << qMakePair(QString::fromLatin1("File Path"),
                     QDir::toNativeSeparators(document->fileName()))
        << qMakePair(QString::fromLatin1("Last Modified"), toString(document->lastModified()))
        << qMakePair(QString::fromLatin1("Revision"), toString(document->revision()))
        << qMakePair(QString::fromLatin1("Editor Revision"), toString(document->editorRevision()))
        << qMakePair(QString::fromLatin1("Check Mode"), toString(document->checkMode()))
        << qMakePair(QString::fromLatin1("Tokenized"), toString(document->isTokenized()))
        << qMakePair(QString::fromLatin1("Parsed"), toString(document->isParsed()))
        << qMakePair(QString::fromLatin1("Project Parts"), partsForFile(document->fileName()))
        ;
    m_docGenericInfoModel->configure(table);
    resizeColumns<KeyValueModel>(m_ui->docGeneralView);

    // Includes
    m_docIncludesModel->configure(document->resolvedIncludes() + document->unresolvedIncludes());
    resizeColumns<IncludesModel>(m_ui->docIncludesView);
    m_ui->docTab->setTabText(DocumentIncludesTab,
        docTabName(DocumentIncludesTab, m_docIncludesModel->rowCount()));

    // Diagnostic Messages
    m_docDiagnosticMessagesModel->configure(document->diagnosticMessages());
    resizeColumns<DiagnosticMessagesModel>(m_ui->docDiagnosticMessagesView);
    m_ui->docTab->setTabText(DocumentDiagnosticsTab,
        docTabName(DocumentDiagnosticsTab, m_docDiagnosticMessagesModel->rowCount()));

    // Macros
    m_docMacrosModel->configure(document->definedMacros());
    resizeColumns<MacrosModel>(m_ui->docDefinedMacrosView);
    m_ui->docTab->setTabText(DocumentDefinedMacrosTab,
        docTabName(DocumentDefinedMacrosTab, m_docMacrosModel->rowCount()));

    // Source
    m_ui->docPreprocessedSourceEdit->setPlainText(QString::fromUtf8(document->utf8Source()));

    // Symbols
    m_docSymbolsModel->configure(document);
    resizeColumns<SymbolsModel>(m_ui->docSymbolsView);

    // Tokens
    m_docTokensModel->configure(document->translationUnit());
    resizeColumns<TokensModel>(m_ui->docTokensView);
    m_ui->docTab->setTabText(DocumentTokensTab,
        docTabName(DocumentTokensTab, m_docTokensModel->rowCount()));
}

enum ProjectPartTabs {
    ProjectPartGeneralTab,
    ProjectPartFilesTab,
    ProjectPartDefinesTab,
    ProjectPartIncludePathsTab,
    ProjectPartFrameworkPathsTab,
    ProjectPartPrecompiledHeadersTab
};

static QString partTabName(int tabIndex, int numberOfEntries = -1)
{
    const char *names[] = {
        "&General",
        "Project &Files",
        "&Defines",
        "&Include Paths",
        "F&ramework Paths",
        "Pre&compiled Headers"
    };
    QString result = QLatin1String(names[tabIndex]);
    if (numberOfEntries != -1)
        result += QString::fromLatin1(" (%1)").arg(numberOfEntries);
    return result;
}

void CppCodeModelInspectorDialog::clearProjectPartData()
{
    m_partGenericInfoModel->clear();

    m_ui->partProjectFilesEdit->setPlainText(QString());
    m_ui->projectPartTab->setTabText(ProjectPartFilesTab, partTabName(ProjectPartFilesTab));

    m_ui->partToolchainDefinesEdit->setPlainText(QString());
    m_ui->partProjectDefinesEdit->setPlainText(QString());
    m_ui->projectPartTab->setTabText(ProjectPartDefinesTab, partTabName(ProjectPartDefinesTab));

    m_ui->partIncludePathsEdit->setPlainText(QString());
    m_ui->projectPartTab->setTabText(ProjectPartIncludePathsTab,
                                     partTabName(ProjectPartIncludePathsTab));

    m_ui->partFrameworkPathsEdit->setPlainText(QString());
    m_ui->projectPartTab->setTabText(ProjectPartFrameworkPathsTab,
                                     partTabName(ProjectPartFrameworkPathsTab));

    m_ui->partPrecompiledHeadersEdit->setPlainText(QString());
    m_ui->projectPartTab->setTabText(ProjectPartPrecompiledHeadersTab,
                                     partTabName(ProjectPartPrecompiledHeadersTab));
}

void CppCodeModelInspectorDialog::updateProjectPartData(const ProjectPart::Ptr &part)
{
    QTC_ASSERT(part, return);

    // General
    QString projectName = QLatin1String("<None>");
    QString projectFilePath = QLatin1String("<None>");
    if (ProjectExplorer::Project *project = part->project) {
        projectName = project->displayName();
        projectFilePath = project->projectFilePath();
    }
    KeyValueModel::Table table = KeyValueModel::Table()
        << qMakePair(QString::fromLatin1("Project Part Name"), part->displayName)
        << qMakePair(QString::fromLatin1("Project Part File"),
                     QDir::toNativeSeparators(part->projectFile))
        << qMakePair(QString::fromLatin1("Project Name"), projectName)
        << qMakePair(QString::fromLatin1("Project File"),
                     QDir::toNativeSeparators(projectFilePath))
        << qMakePair(QString::fromLatin1("C Version"), toString(part->cVersion))
        << qMakePair(QString::fromLatin1("CXX Version"), toString(part->cxxVersion))
        << qMakePair(QString::fromLatin1("CXX Extensions"), toString(part->cxxExtensions))
        << qMakePair(QString::fromLatin1("Qt Version"), toString(part->qtVersion))
        ;
    if (!part->projectConfigFile.isEmpty())
        table.prepend(qMakePair(QString::fromLatin1("Project Config File"),
                                part->projectConfigFile));
    m_partGenericInfoModel->configure(table);
    resizeColumns<KeyValueModel>(m_ui->partGeneralView);

    // Project Files
    m_ui->partProjectFilesEdit->setPlainText(toString(part->files));
    m_ui->projectPartTab->setTabText(ProjectPartFilesTab,
        partTabName(ProjectPartFilesTab, part->files.size()));

    // Defines
    const QList<QByteArray> defineLines = part->toolchainDefines.split('\n')
        + part->projectDefines.split('\n');
    int numberOfDefines = 0;
    foreach (const QByteArray &line, defineLines) {
        if (line.startsWith("#define "))
            ++numberOfDefines;
    }
    m_ui->partToolchainDefinesEdit->setPlainText(QString::fromUtf8(part->toolchainDefines));
    m_ui->partProjectDefinesEdit->setPlainText(QString::fromUtf8(part->projectDefines));
    m_ui->projectPartTab->setTabText(ProjectPartDefinesTab,
        partTabName(ProjectPartDefinesTab, numberOfDefines));

    // Include Paths
    m_ui->partIncludePathsEdit->setPlainText(pathListToString(part->includePaths));
    m_ui->projectPartTab->setTabText(ProjectPartIncludePathsTab,
        partTabName(ProjectPartIncludePathsTab, part->includePaths.size()));

    // Framework Paths
    m_ui->partFrameworkPathsEdit->setPlainText(pathListToString(part->frameworkPaths));
    m_ui->projectPartTab->setTabText(ProjectPartFrameworkPathsTab,
        partTabName(ProjectPartFrameworkPathsTab, part->frameworkPaths.size()));

    // Precompiled Headers
    m_ui->partPrecompiledHeadersEdit->setPlainText(pathListToString(part->precompiledHeaders));
    m_ui->projectPartTab->setTabText(ProjectPartPrecompiledHeadersTab,
        partTabName(ProjectPartPrecompiledHeadersTab, part->precompiledHeaders.size()));
}

bool CppCodeModelInspectorDialog::event(QEvent *e)
{
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            close();
            return false;
        }
    }
    return QDialog::event(e);
}

} // namespace Internal
} // namespace CppEditor

#include "cppcodemodelinspectordialog.moc"
