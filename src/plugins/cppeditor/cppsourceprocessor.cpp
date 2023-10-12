// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppsourceprocessor.h"

#include "cppeditortr.h"
#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QLoggingCategory>
#include <QTextCodec>

/*!
 * \class CppEditor::Internal::CppSourceProcessor
 * \brief The CppSourceProcessor class updates set of indexed C++ files.
 *
 * Working copy ensures that documents with most recent copy placed in memory will be parsed
 * correctly.
 *
 * \sa CPlusPlus::Document
 * \sa CppEditor::WorkingCopy
 */

using namespace CPlusPlus;
using namespace Utils;

using Message = Document::DiagnosticMessage;

namespace CppEditor::Internal {

static Q_LOGGING_CATEGORY(log, "qtc.cppeditor.sourceprocessor", QtWarningMsg)

namespace {

inline QByteArray generateFingerPrint(const QList<CPlusPlus::Macro> &definedMacros,
                                      const QByteArray &code)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(code);
    for (const CPlusPlus::Macro &macro : definedMacros) {
        if (macro.isHidden()) {
            static const QByteArray undef("#undef ");
            hash.addData(undef);
            hash.addData(macro.name());
        } else {
            static const QByteArray def("#define ");
            hash.addData(macro.name());
            hash.addData(" ", 1);
            hash.addData(def);
            hash.addData(macro.definitionText());
        }
        hash.addData("\n", 1);
    }
    return hash.result();
}

inline Message messageNoSuchFile(Document::Ptr &document, const FilePath &filePath, unsigned line)
{
    const QString text = Tr::tr("%1: No such file or directory").arg(filePath.displayName());
    return Message(Message::Warning, document->filePath(), line, /*column =*/ 0, text);
}

inline Message messageNoFileContents(Document::Ptr &document, const FilePath &filePath,
                                     unsigned line)
{
    const QString text = Tr::tr("%1: Could not get file contents").arg(filePath.displayName());
    return Message(Message::Warning, document->filePath(), line, /*column =*/ 0, text);
}

inline const CPlusPlus::Macro revision(const WorkingCopy &workingCopy,
                                       const CPlusPlus::Macro &macro)
{
    CPlusPlus::Macro newMacro(macro);
    if (const auto entry = workingCopy.get(macro.filePath()))
        newMacro.setFileRevision(entry->second);
    return newMacro;
}

} // anonymous namespace

CppSourceProcessor::CppSourceProcessor(const Snapshot &snapshot, DocumentCallback documentFinished)
    : m_snapshot(snapshot),
      m_documentFinished(documentFinished),
      m_preprocess(this, &m_env),
      m_languageFeatures(LanguageFeatures::defaultFeatures()),
      m_defaultCodec(Core::EditorManager::defaultTextCodec())
{
    m_preprocess.setKeepComments(true);
}

CppSourceProcessor::~CppSourceProcessor() = default;

void CppSourceProcessor::setCancelChecker(const CppSourceProcessor::CancelChecker &cancelChecker)
{
    m_preprocess.setCancelChecker(cancelChecker);
}

void CppSourceProcessor::setWorkingCopy(const WorkingCopy &workingCopy)
{ m_workingCopy = workingCopy; }

void CppSourceProcessor::setHeaderPaths(const ProjectExplorer::HeaderPaths &headerPaths)
{
    using ProjectExplorer::HeaderPathType;
    m_headerPaths.clear();

    for (const auto &path : headerPaths) {
         if (path.type == HeaderPathType::Framework )
            addFrameworkPath(path);
        else
            m_headerPaths.append({path.path, path.type});
    }
}

void CppSourceProcessor::setLanguageFeatures(const LanguageFeatures languageFeatures)
{
    m_languageFeatures = languageFeatures;
}

// Add the given framework path, and expand private frameworks.
//
// Example:
//  <framework-path>/ApplicationServices.framework
// has private frameworks in:
//  <framework-path>/ApplicationServices.framework/Frameworks
// if the "Frameworks" folder exists inside the top level framework.
void CppSourceProcessor::addFrameworkPath(const ProjectExplorer::HeaderPath &frameworkPath)
{
    QTC_ASSERT(frameworkPath.type == ProjectExplorer::HeaderPathType::Framework, return);

    // The algorithm below is a bit too eager, but that's because we're not getting
    // in the frameworks we're linking against. If we would have that, then we could
    // add only those private frameworks.
    const auto cleanFrameworkPath = ProjectExplorer::HeaderPath::makeFramework(
                frameworkPath.path);
    if (!m_headerPaths.contains(cleanFrameworkPath))
        m_headerPaths.append(cleanFrameworkPath);

    const QDir frameworkDir(cleanFrameworkPath.path);
    const QStringList filter = QStringList("*.framework");
    const QList<QFileInfo> frameworks = frameworkDir.entryInfoList(filter);
    for (const QFileInfo &framework : frameworks) {
        if (!framework.isDir())
            continue;
        const QFileInfo privateFrameworks(framework.absoluteFilePath(),
                                          QLatin1String("Frameworks"));
        if (privateFrameworks.exists() && privateFrameworks.isDir())
            addFrameworkPath(ProjectExplorer::HeaderPath::makeFramework(
                                 privateFrameworks.absoluteFilePath()));
    }
}

void CppSourceProcessor::setTodo(const QSet<QString> &files)
{
    m_todo = files;
}

void CppSourceProcessor::run(const FilePath &filePath,
                             const FilePaths &initialIncludes)
{
    sourceNeeded(0, filePath, IncludeGlobal, initialIncludes);
}

void CppSourceProcessor::removeFromCache(const FilePath &filePath)
{
    m_snapshot.remove(filePath);
}

void CppSourceProcessor::resetEnvironment()
{
    m_env.reset();
    m_processed.clear();
    m_included.clear();
}

bool CppSourceProcessor::getFileContents(const FilePath &absoluteFilePath,
                                         QByteArray *contents,
                                         unsigned *revision) const
{
    if (absoluteFilePath.isEmpty() || !contents || !revision)
        return false;

    // Get from working copy
    if (const auto entry = m_workingCopy.get(absoluteFilePath)) {
        *contents = entry->first;
        *revision = entry->second;
        return true;
    }

    // Get from file
    *revision = 0;
    QString error;
    if (Utils::TextFileFormat::readFileUTF8(absoluteFilePath,
                                            m_defaultCodec,
                                            contents,
                                            &error)
        != Utils::TextFileFormat::ReadSuccess) {
        qWarning("Error reading file \"%s\": \"%s\".", qPrintable(absoluteFilePath.toString()),
                 qPrintable(error));
        return false;
    }
    contents->replace("\r\n", "\n");
    return true;
}

bool CppSourceProcessor::checkFile(const FilePath &absoluteFilePath) const
{
    if (absoluteFilePath.isEmpty()
            || m_included.contains(absoluteFilePath)
            || m_workingCopy.get(absoluteFilePath)) {
        return true;
    }

    return absoluteFilePath.isReadableFile();
}

/// Resolve the given file name to its absolute path w.r.t. the include type.
FilePath CppSourceProcessor::resolveFile(const FilePath &filePath, IncludeType type)
{
    if (isInjectedFile(filePath.path()))
        return filePath;

    if (filePath.isAbsolutePath())
        return checkFile(filePath) ? filePath : FilePath();

    if (m_currentDoc) {
        if (type == IncludeLocal) {
            const FilePath currentFilePath = m_currentDoc->filePath();
            const FilePath path = currentFilePath.resolvePath("../" + filePath.path());
            if (checkFile(path))
                return path;
            // Fall through! "16.2 Source file inclusion" from the standard states to continue
            // searching as if this would be a global include.

        } else if (type == IncludeNext) {
            const FilePath currentFilePath = m_currentDoc->filePath();
            const FilePath currentDirPath = currentFilePath.parentDir();
            auto headerPathsEnd = m_headerPaths.end();
            auto headerPathsIt = m_headerPaths.begin();
            for (; headerPathsIt != headerPathsEnd; ++headerPathsIt) {
                if (headerPathsIt->path == currentDirPath.path()) {
                    ++headerPathsIt;
                    return resolveFile_helper(filePath, headerPathsIt);
                }
            }
        }
    }

    const auto it = m_fileNameCache.constFind(filePath);
    if (it != m_fileNameCache.constEnd())
        return it.value();
    const FilePath fn = resolveFile_helper(filePath, m_headerPaths.begin());
    if (!fn.isEmpty())
        m_fileNameCache.insert(filePath, fn);
    return fn;
}

FilePath CppSourceProcessor::resolveFile_helper(const FilePath &filePath,
                                                ProjectExplorer::HeaderPaths::Iterator headerPathsIt)
{
    const QString fileName = filePath.path();
    auto headerPathsEnd = m_headerPaths.end();
    const int index = fileName.indexOf(QLatin1Char('/'));
    for (; headerPathsIt != headerPathsEnd; ++headerPathsIt) {
        if (!headerPathsIt->path.isNull()) {
            FilePath path;
            if (headerPathsIt->type == ProjectExplorer::HeaderPathType::Framework) {
                if (index == -1)
                    continue;
                path = FilePath::fromString(headerPathsIt->path).pathAppended(fileName.left(index)
                       + QLatin1String(".framework/Headers/") + fileName.mid(index + 1));
            } else {
                path = FilePath::fromString(headerPathsIt->path) /  fileName;
            }
            if (m_workingCopy.get(path) || checkFile(path))
                return path;
        }
    }

    return {};
}

void CppSourceProcessor::macroAdded(const CPlusPlus::Macro &macro)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->appendMacro(macro);
}

void CppSourceProcessor::passedMacroDefinitionCheck(int bytesOffset, int utf16charsOffset,
                                                    int line, const CPlusPlus::Macro &macro)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro),
                              bytesOffset, macro.name().length(),
                              utf16charsOffset, macro.nameToQString().size(),
                              line, QVector<MacroArgumentReference>());
}

void CppSourceProcessor::failedMacroDefinitionCheck(int bytesOffset, int utf16charOffset,
                                                    const ByteArrayRef &name)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addUndefinedMacroUse(QByteArray(name.start(), name.size()),
                                       bytesOffset, utf16charOffset);
}

void CppSourceProcessor::notifyMacroReference(int bytesOffset, int utf16charOffset,
                                              int line, const CPlusPlus::Macro &macro)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro),
                              bytesOffset, macro.name().length(),
                              utf16charOffset, macro.nameToQString().size(),
                              line, QVector<MacroArgumentReference>());
}

void CppSourceProcessor::startExpandingMacro(int bytesOffset, int utf16charOffset,
                                             int line, const CPlusPlus::Macro &macro,
                                             const QVector<MacroArgumentReference> &actuals)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro),
                              bytesOffset, macro.name().length(),
                              utf16charOffset, macro.nameToQString().size(),
                              line, actuals);
}

void CppSourceProcessor::stopExpandingMacro(int, const CPlusPlus::Macro &)
{
    if (!m_currentDoc)
        return;
}

void CppSourceProcessor::markAsIncludeGuard(const QByteArray &macroName)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->setIncludeGuardMacroName(macroName);
}

void CppSourceProcessor::mergeEnvironment(Document::Ptr doc)
{
    if (!doc)
        return;

    if (!Utils::insert(m_processed, doc->filePath()))
        return;

    const QList<Document::Include> includes = doc->resolvedIncludes();
    for (const Document::Include &incl : includes) {
        const FilePath includedFile = incl.resolvedFileName();

        if (Document::Ptr includedDoc = m_snapshot.document(includedFile))
            mergeEnvironment(includedDoc);
        else if (!m_included.contains(includedFile))
            run(includedFile);
    }

    m_env.addMacros(doc->definedMacros());
}

void CppSourceProcessor::startSkippingBlocks(int utf16charsOffset)
{
    if (m_currentDoc)
        m_currentDoc->startSkippingBlocks(utf16charsOffset);
}

void CppSourceProcessor::stopSkippingBlocks(int utf16charsOffset)
{
    if (m_currentDoc)
        m_currentDoc->stopSkippingBlocks(utf16charsOffset);
}

void CppSourceProcessor::sourceNeeded(int line, const FilePath &filePath, IncludeType type,
                                      const FilePaths &initialIncludes)
{
    if (filePath.isEmpty())
        return;

    const FilePath absoluteFilePath = resolveFile(filePath, type);

    if (m_currentDoc) {
        m_currentDoc->addIncludeFile(Document::Include(filePath.toString(), absoluteFilePath, line, type));
        if (absoluteFilePath.isEmpty()) {
            m_currentDoc->addDiagnosticMessage(messageNoSuchFile(m_currentDoc, filePath, line));
            return;
        }
    }
    if (m_included.contains(absoluteFilePath))
        return; // We've already seen this file.
    if (!isInjectedFile(absoluteFilePath.path()))
        m_included.insert(absoluteFilePath);

    // Already in snapshot? Use it!
    if (Document::Ptr document = m_snapshot.document(absoluteFilePath)) {
        mergeEnvironment(document);
        return;
    }

    if (fileSizeExceedsLimit(absoluteFilePath, m_fileSizeLimitInMb))
        return; // TODO: Add diagnostic message

    // Otherwise get file contents
    unsigned editorRevision = 0;
    QByteArray contents;
    const bool gotFileContents = getFileContents(absoluteFilePath, &contents, &editorRevision);
    if (m_currentDoc && !gotFileContents) {
        m_currentDoc->addDiagnosticMessage(messageNoFileContents(m_currentDoc, filePath, line));
        return;
    }

    qCDebug(log) << "Parsing:" << absoluteFilePath.toString() << "contents:" << contents.size() << "bytes";

    Document::Ptr document = Document::create(absoluteFilePath);
    document->setEditorRevision(editorRevision);
    document->setLanguageFeatures(m_languageFeatures);
    for (const FilePath &include : initialIncludes) {
        m_included.insert(include);
        Document::Include inc(include.toString(), include, 0, IncludeLocal);
        document->addIncludeFile(inc);
    }
    if (absoluteFilePath.exists())
        document->setLastModified(absoluteFilePath.lastModified());

    const Document::Ptr previousDocument = switchCurrentDocument(document);
    const QByteArray preprocessedCode = m_preprocess.run(absoluteFilePath, contents);
//    {
//        QByteArray b(preprocessedCode); b.replace("\n", "<<<\n");
//        qDebug("Preprocessed code for \"%s\": [[%s]]", fileName.toUtf8().constData(), b.constData());
//    }
    document->setFingerprint(generateFingerPrint(document->definedMacros(), preprocessedCode));

    // Re-use document from global snapshot if possible
    Document::Ptr globalDocument = m_globalSnapshot.document(absoluteFilePath);
    if (globalDocument && globalDocument->fingerprint() == document->fingerprint()) {
        switchCurrentDocument(previousDocument);
        mergeEnvironment(globalDocument);
        m_snapshot.insert(globalDocument);
        m_todo.remove(absoluteFilePath.toString());
        return;
    }

    // Otherwise process the document
    document->setUtf8Source(preprocessedCode);
    document->keepSourceAndAST();
    document->tokenize();
    document->check(m_workingCopy.get(document->filePath()) ? Document::FullCheck
                                                            : Document::FastCheck);

    m_documentFinished(document);

    m_snapshot.insert(document);
    m_todo.remove(absoluteFilePath.toString());
    switchCurrentDocument(previousDocument);
}

void CppSourceProcessor::setFileSizeLimitInMb(int fileSizeLimitInMb)
{
    m_fileSizeLimitInMb = fileSizeLimitInMb;
}

Document::Ptr CppSourceProcessor::switchCurrentDocument(Document::Ptr doc)
{
    const Document::Ptr previousDoc = m_currentDoc;
    m_currentDoc = doc;
    return previousDoc;
}

} // namespace CppEditor::Internal
