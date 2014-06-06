#include "cppsourceprocessor.h"

#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/textfileformat.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QTextCodec>

/*!
 * \class CppTools::Internal::CppSourceProcessor
 * \brief The CppSourceProcessor class updates set of indexed C++ files.
 *
 * Working copy ensures that documents with most recent copy placed in memory will be parsed
 * correctly.
 *
 * \sa CPlusPlus::Document
 * \sa CppTools::CppModelManagerInterface::WorkingCopy
 */

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

typedef Document::DiagnosticMessage Message;

namespace {

inline QByteArray generateFingerPrint(const QList<Macro> &definedMacros, const QByteArray &code)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(code);
    foreach (const Macro &macro, definedMacros) {
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

inline Message messageNoSuchFile(Document::Ptr &document, const QString &fileName, unsigned line)
{
    const QString text = QCoreApplication::translate(
        "CppSourceProcessor", "%1: No such file or directory").arg(fileName);
    return Message(Message::Warning, document->fileName(), line, /*column =*/ 0, text);
}

inline Message messageNoFileContents(Document::Ptr &document, const QString &fileName,
                                     unsigned line)
{
    const QString text = QCoreApplication::translate(
        "CppSourceProcessor", "%1: Could not get file contents").arg(fileName);
    return Message(Message::Warning, document->fileName(), line, /*column =*/ 0, text);
}

inline const Macro revision(const CppModelManagerInterface::WorkingCopy &workingCopy,
                            const Macro &macro)
{
    Macro newMacro(macro);
    newMacro.setFileRevision(workingCopy.get(macro.fileName()).second);
    return newMacro;
}

} // anonymous namespace

CppSourceProcessor::CppSourceProcessor(const Snapshot &snapshot, DocumentCallback documentFinished)
    : m_snapshot(snapshot),
      m_documentFinished(documentFinished),
      m_dumpFileNameWhileParsing(false),
      m_preprocess(this, &m_env),
      m_revision(0),
      m_defaultCodec(Core::EditorManager::defaultTextCodec())
{
    m_preprocess.setKeepComments(true);
}

CppSourceProcessor::~CppSourceProcessor()
{ }

void CppSourceProcessor::setRevision(unsigned revision)
{ m_revision = revision; }

void CppSourceProcessor::setWorkingCopy(const CppModelManagerInterface::WorkingCopy &workingCopy)
{ m_workingCopy = workingCopy; }

void CppSourceProcessor::setIncludePaths(const QStringList &includePaths)
{
    m_includePaths.clear();

    for (int i = 0; i < includePaths.size(); ++i) {
        const QString &path = includePaths.at(i);

        if (Utils::HostOsInfo::isMacHost()) {
            if (i + 1 < includePaths.size() && path.endsWith(QLatin1String(".framework/Headers"))) {
                const QFileInfo pathInfo(path);
                const QFileInfo frameworkFileInfo(pathInfo.path());
                const QString frameworkName = frameworkFileInfo.baseName();

                const QFileInfo nextIncludePath = includePaths.at(i + 1);
                if (nextIncludePath.fileName() == frameworkName) {
                    // We got a QtXXX.framework/Headers followed by $QTDIR/include/QtXXX.
                    // In this case we prefer to include files from $QTDIR/include/QtXXX.
                    continue;
                }
            }
        }
        m_includePaths.append(cleanPath(path));
    }
}

void CppSourceProcessor::setFrameworkPaths(const QStringList &frameworkPaths)
{
    m_frameworkPaths.clear();
    foreach (const QString &frameworkPath, frameworkPaths)
        addFrameworkPath(frameworkPath);
}

// Add the given framework path, and expand private frameworks.
//
// Example:
//  <framework-path>/ApplicationServices.framework
// has private frameworks in:
//  <framework-path>/ApplicationServices.framework/Frameworks
// if the "Frameworks" folder exists inside the top level framework.
void CppSourceProcessor::addFrameworkPath(const QString &frameworkPath)
{
    // The algorithm below is a bit too eager, but that's because we're not getting
    // in the frameworks we're linking against. If we would have that, then we could
    // add only those private frameworks.
    const QString cleanFrameworkPath = cleanPath(frameworkPath);
    if (!m_frameworkPaths.contains(cleanFrameworkPath))
        m_frameworkPaths.append(cleanFrameworkPath);

    const QDir frameworkDir(cleanFrameworkPath);
    const QStringList filter = QStringList() << QLatin1String("*.framework");
    foreach (const QFileInfo &framework, frameworkDir.entryInfoList(filter)) {
        if (!framework.isDir())
            continue;
        const QFileInfo privateFrameworks(framework.absoluteFilePath(),
                                          QLatin1String("Frameworks"));
        if (privateFrameworks.exists() && privateFrameworks.isDir())
            addFrameworkPath(privateFrameworks.absoluteFilePath());
    }
}

void CppSourceProcessor::setTodo(const QStringList &files)
{ m_todo = QSet<QString>::fromList(files); }

void CppSourceProcessor::run(const QString &fileName)
{
    sourceNeeded(0, fileName, IncludeGlobal);
}

void CppSourceProcessor::removeFromCache(const QString &fileName)
{
    m_snapshot.remove(fileName);
}

void CppSourceProcessor::resetEnvironment()
{
    m_env.reset();
    m_processed.clear();
    m_included.clear();
}

bool CppSourceProcessor::getFileContents(const QString &absoluteFilePath,
                                         QByteArray *contents,
                                         unsigned *revision) const
{
    if (absoluteFilePath.isEmpty() || !contents || !revision)
        return false;

    // Get from working copy
    if (m_workingCopy.contains(absoluteFilePath)) {
        const QPair<QByteArray, unsigned> entry = m_workingCopy.get(absoluteFilePath);
        *contents = entry.first;
        *revision = entry.second;
        return true;
    }

    // Get from file
    *revision = 0;
    QString error;
    if (Utils::TextFileFormat::readFileUTF8(absoluteFilePath, m_defaultCodec, contents, &error)
            != Utils::TextFileFormat::ReadSuccess) {
        qWarning("Error reading file \"%s\": \"%s\".", qPrintable(absoluteFilePath),
                 qPrintable(error));
        return false;
    }
    return true;
}

bool CppSourceProcessor::checkFile(const QString &absoluteFilePath) const
{
    if (absoluteFilePath.isEmpty()
            || m_included.contains(absoluteFilePath)
            || m_workingCopy.contains(absoluteFilePath)) {
        return true;
    }

    const QFileInfo fileInfo(absoluteFilePath);
    return fileInfo.isFile() && fileInfo.isReadable();
}

/// Resolve the given file name to its absolute path w.r.t. the include type.
QString CppSourceProcessor::resolveFile(const QString &fileName, IncludeType type)
{
    if (type == IncludeGlobal) {
        QHash<QString, QString>::ConstIterator it = m_fileNameCache.find(fileName);
        if (it != m_fileNameCache.end())
            return it.value();
        const QString fn = resolveFile_helper(fileName, type);
        m_fileNameCache.insert(fileName, fn);
        return fn;
    }

    // IncludeLocal, IncludeNext
    return resolveFile_helper(fileName, type);
}

QString CppSourceProcessor::cleanPath(const QString &path)
{
    QString result = QDir::cleanPath(path);
    const QChar slash(QLatin1Char('/'));
    if (!result.endsWith(slash))
        result.append(slash);
    return result;
}

QString CppSourceProcessor::resolveFile_helper(const QString &fileName, IncludeType type)
{
    if (isInjectedFile(fileName))
        return fileName;

    if (QFileInfo(fileName).isAbsolute())
        return checkFile(fileName) ? fileName : QString();

    if (type == IncludeLocal && m_currentDoc) {
        const QFileInfo currentFileInfo(m_currentDoc->fileName());
        const QString path = cleanPath(currentFileInfo.absolutePath()) + fileName;
        if (checkFile(path))
            return path;
        // Fall through! "16.2 Source file inclusion" from the standard states to continue
        // searching as if this would be a global include.
    }

    foreach (const QString &includePath, m_includePaths) {
        const QString path = includePath + fileName;
        if (m_workingCopy.contains(path) || checkFile(path))
            return path;
    }

    const int index = fileName.indexOf(QLatin1Char('/'));
    if (index != -1) {
        const QString frameworkName = fileName.left(index);
        const QString name = frameworkName + QLatin1String(".framework/Headers/")
            + fileName.mid(index + 1);

        foreach (const QString &frameworkPath, m_frameworkPaths) {
            const QString path = frameworkPath + name;
            if (checkFile(path))
                return path;
        }
    }

    return QString();
}

void CppSourceProcessor::macroAdded(const Macro &macro)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->appendMacro(macro);
}

void CppSourceProcessor::passedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                                    unsigned line, const Macro &macro)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro),
                              bytesOffset, macro.name().length(),
                              utf16charsOffset, macro.nameToQString().size(),
                              line, QVector<MacroArgumentReference>());
}

void CppSourceProcessor::failedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charOffset,
                                                    const ByteArrayRef &name)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addUndefinedMacroUse(QByteArray(name.start(), name.size()),
                                       bytesOffset, utf16charOffset);
}

void CppSourceProcessor::notifyMacroReference(unsigned bytesOffset, unsigned utf16charOffset,
                                              unsigned line, const Macro &macro)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro),
                              bytesOffset, macro.name().length(),
                              utf16charOffset, macro.nameToQString().size(),
                              line, QVector<MacroArgumentReference>());
}

void CppSourceProcessor::startExpandingMacro(unsigned bytesOffset, unsigned utf16charOffset,
                                             unsigned line, const Macro &macro,
                                             const QVector<MacroArgumentReference> &actuals)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro),
                              bytesOffset, macro.name().length(),
                              utf16charOffset, macro.nameToQString().size(),
                              line, actuals);
}

void CppSourceProcessor::stopExpandingMacro(unsigned, const Macro &)
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

    const QString fn = doc->fileName();

    if (m_processed.contains(fn))
        return;

    m_processed.insert(fn);

    foreach (const Document::Include &incl, doc->resolvedIncludes()) {
        const QString includedFile = incl.resolvedFileName();

        if (Document::Ptr includedDoc = m_snapshot.document(includedFile))
            mergeEnvironment(includedDoc);
        else if (!m_included.contains(includedFile))
            run(includedFile);
    }

    m_env.addMacros(doc->definedMacros());
}

void CppSourceProcessor::startSkippingBlocks(unsigned utf16charsOffset)
{
    if (m_currentDoc)
        m_currentDoc->startSkippingBlocks(utf16charsOffset);
}

void CppSourceProcessor::stopSkippingBlocks(unsigned utf16charsOffset)
{
    if (m_currentDoc)
        m_currentDoc->stopSkippingBlocks(utf16charsOffset);
}

void CppSourceProcessor::sourceNeeded(unsigned line, const QString &fileName, IncludeType type)
{
    if (fileName.isEmpty())
        return;

    QString absoluteFileName = resolveFile(fileName, type);
    absoluteFileName = QDir::cleanPath(absoluteFileName);
    if (m_currentDoc) {
        m_currentDoc->addIncludeFile(Document::Include(fileName, absoluteFileName, line, type));
        if (absoluteFileName.isEmpty()) {
            m_currentDoc->addDiagnosticMessage(messageNoSuchFile(m_currentDoc, fileName, line));
            return;
        }
    }
    if (m_included.contains(absoluteFileName))
        return; // We've already seen this file.
    if (!isInjectedFile(absoluteFileName))
        m_included.insert(absoluteFileName);

    // Already in snapshot? Use it!
    if (Document::Ptr document = m_snapshot.document(absoluteFileName)) {
        mergeEnvironment(document);
        return;
    }

    // Otherwise get file contents
    unsigned editorRevision = 0;
    QByteArray contents;
    const bool gotFileContents = getFileContents(absoluteFileName, &contents, &editorRevision);
    if (m_currentDoc && !gotFileContents) {
        m_currentDoc->addDiagnosticMessage(messageNoFileContents(m_currentDoc, fileName, line));
        return;
    }

    if (m_dumpFileNameWhileParsing)
        qDebug() << "Parsing:" << absoluteFileName << "contents:" << contents.size() << "bytes";

    Document::Ptr document = Document::create(absoluteFileName);
    document->setRevision(m_revision);
    document->setEditorRevision(editorRevision);
    const QFileInfo info(absoluteFileName);
    if (info.exists())
        document->setLastModified(info.lastModified());

    const Document::Ptr previousDocument = switchCurrentDocument(document);
    const QByteArray preprocessedCode = m_preprocess.run(absoluteFileName, contents);
//    {
//        QByteArray b(preprocessedCode); b.replace("\n", "<<<\n");
//        qDebug("Preprocessed code for \"%s\": [[%s]]", fileName.toUtf8().constData(), b.constData());
//    }
    document->setFingerprint(generateFingerPrint(document->definedMacros(), preprocessedCode));

    // Re-use document from global snapshot if possible
    Document::Ptr globalDocument = m_globalSnapshot.document(absoluteFileName);
    if (globalDocument && globalDocument->fingerprint() == document->fingerprint()) {
        switchCurrentDocument(previousDocument);
        mergeEnvironment(globalDocument);
        m_snapshot.insert(globalDocument);
        m_todo.remove(absoluteFileName);
        return;
    }

    // Otherwise process the document
    document->setUtf8Source(preprocessedCode);
    document->keepSourceAndAST();
    document->tokenize();
    document->check(m_workingCopy.contains(document->fileName()) ? Document::FullCheck
                                                                 : Document::FastCheck);

    m_documentFinished(document);

    m_snapshot.insert(document);
    m_todo.remove(absoluteFileName);
    switchCurrentDocument(previousDocument);
}

Document::Ptr CppSourceProcessor::switchCurrentDocument(Document::Ptr doc)
{
    const Document::Ptr previousDoc = m_currentDoc;
    m_currentDoc = doc;
    return previousDoc;
}
