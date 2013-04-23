#include "cppmodelmanager.h"
#include "cpppreprocessor.h"

#include <utils/hostosinfo.h>

#include <QCoreApplication>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

CppPreprocessor::CppPreprocessor(QPointer<CppModelManager> modelManager, bool dumpFileNameWhileParsing)
    : m_snapshot(modelManager->snapshot()),
      m_modelManager(modelManager),
      m_dumpFileNameWhileParsing(dumpFileNameWhileParsing),
      m_preprocess(this, &m_env),
      m_revision(0)
{
    m_preprocess.setKeepComments(true);
}

CppPreprocessor::~CppPreprocessor()
{ }

void CppPreprocessor::setRevision(unsigned revision)
{ m_revision = revision; }

void CppPreprocessor::setWorkingCopy(const CppModelManagerInterface::WorkingCopy &workingCopy)
{ m_workingCopy = workingCopy; }

void CppPreprocessor::setIncludePaths(const QStringList &includePaths)
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

void CppPreprocessor::setFrameworkPaths(const QStringList &frameworkPaths)
{
    m_frameworkPaths.clear();

    foreach (const QString &frameworkPath, frameworkPaths) {
        addFrameworkPath(frameworkPath);
    }
}

// Add the given framework path, and expand private frameworks.
//
// Example:
//  <framework-path>/ApplicationServices.framework
// has private frameworks in:
//  <framework-path>/ApplicationServices.framework/Frameworks
// if the "Frameworks" folder exists inside the top level framework.
void CppPreprocessor::addFrameworkPath(const QString &frameworkPath)
{
    // The algorithm below is a bit too eager, but that's because we're not getting
    // in the frameworks we're linking against. If we would have that, then we could
    // add only those private frameworks.
    QString cleanFrameworkPath = cleanPath(frameworkPath);
    if (!m_frameworkPaths.contains(cleanFrameworkPath))
        m_frameworkPaths.append(cleanFrameworkPath);

    const QDir frameworkDir(cleanFrameworkPath);
    const QStringList filter = QStringList() << QLatin1String("*.framework");
    foreach (const QFileInfo &framework, frameworkDir.entryInfoList(filter)) {
        if (!framework.isDir())
            continue;
        const QFileInfo privateFrameworks(framework.absoluteFilePath(), QLatin1String("Frameworks"));
        if (privateFrameworks.exists() && privateFrameworks.isDir())
            addFrameworkPath(privateFrameworks.absoluteFilePath());
    }
}

void CppPreprocessor::setTodo(const QStringList &files)
{ m_todo = QSet<QString>::fromList(files); }

namespace {
class Process: public std::unary_function<Document::Ptr, void>
{
    QPointer<CppModelManager> _modelManager;
    Document::Ptr _doc;
    Document::CheckMode _mode;

public:
    Process(QPointer<CppModelManager> modelManager,
            Document::Ptr doc,
            const CppModelManager::WorkingCopy &workingCopy)
        : _modelManager(modelManager),
          _doc(doc),
          _mode(Document::FastCheck)
    {

        if (workingCopy.contains(_doc->fileName()))
            _mode = Document::FullCheck;
    }

    void operator()()
    {
        _doc->check(_mode);

        if (_modelManager)
            _modelManager->emitDocumentUpdated(_doc);

        _doc->releaseSourceAndAST();
    }
};
} // end of anonymous namespace

void CppPreprocessor::run(const QString &fileName)
{
    sourceNeeded(0, fileName, IncludeGlobal);
}

void CppPreprocessor::removeFromCache(const QString &fileName)
{
    m_snapshot.remove(fileName);
}

void CppPreprocessor::resetEnvironment()
{
    m_env.reset();
    m_processed.clear();
}

void CppPreprocessor::getFileContents(const QString &absoluteFilePath,
                                      QString *contents,
                                      unsigned *revision) const
{
    if (absoluteFilePath.isEmpty())
        return;

    if (m_workingCopy.contains(absoluteFilePath)) {
        QPair<QString, unsigned> entry = m_workingCopy.get(absoluteFilePath);
        if (contents)
            *contents = entry.first;
        if (revision)
            *revision = entry.second;
        return;
    }

    QFile file(absoluteFilePath);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextCodec *defaultCodec = Core::EditorManager::instance()->defaultTextCodec();
        QTextStream stream(&file);
        stream.setCodec(defaultCodec);
        if (contents)
            *contents = stream.readAll();
        if (revision)
            *revision = 0;
        file.close();
    }
}

bool CppPreprocessor::checkFile(const QString &absoluteFilePath) const
{
    if (absoluteFilePath.isEmpty() || m_included.contains(absoluteFilePath))
        return true;

    QFileInfo fileInfo(absoluteFilePath);
    return fileInfo.isFile() && fileInfo.isReadable();
}

/// Resolve the given file name to its absolute path w.r.t. the include type.
QString CppPreprocessor::resolveFile(const QString &fileName, IncludeType type)
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

QString CppPreprocessor::cleanPath(const QString &path)
{
    QString result = QDir::cleanPath(path);
    const QChar slash(QLatin1Char('/'));
    if (!result.endsWith(slash))
        result.append(slash);
    return result;
}

QString CppPreprocessor::resolveFile_helper(const QString &fileName, IncludeType type)
{
    QFileInfo fileInfo(fileName);
    if (fileName == Preprocessor::configurationFileName || fileInfo.isAbsolute())
        return fileName;

    if (type == IncludeLocal && m_currentDoc) {
        QFileInfo currentFileInfo(m_currentDoc->fileName());
        QString path = cleanPath(currentFileInfo.absolutePath()) + fileName;
        if (checkFile(path))
            return path;
    }

    foreach (const QString &includePath, m_includePaths) {
        QString path = includePath + fileName;
        if (checkFile(path))
            return path;
    }

    int index = fileName.indexOf(QLatin1Char('/'));
    if (index != -1) {
        QString frameworkName = fileName.left(index);
        QString name = frameworkName + QLatin1String(".framework/Headers/") + fileName.mid(index + 1);

        foreach (const QString &frameworkPath, m_frameworkPaths) {
            QString path = frameworkPath + name;
            if (checkFile(path))
                return path;
        }
    }

    //qDebug() << "**** file" << fileName << "not found!";
    return QString();
}

void CppPreprocessor::macroAdded(const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->appendMacro(macro);
}

static inline const Macro revision(const CppModelManagerInterface::WorkingCopy &s, const Macro &macro)
{
    Macro newMacro(macro);
    newMacro.setFileRevision(s.get(macro.fileName()).second);
    return newMacro;
}

void CppPreprocessor::passedMacroDefinitionCheck(unsigned offset, unsigned line, const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro), offset, macro.name().length(), line,
                              QVector<MacroArgumentReference>());
}

void CppPreprocessor::failedMacroDefinitionCheck(unsigned offset, const ByteArrayRef &name)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addUndefinedMacroUse(QByteArray(name.start(), name.size()), offset);
}

void CppPreprocessor::notifyMacroReference(unsigned offset, unsigned line, const Macro &macro)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro), offset, macro.name().length(), line,
                              QVector<MacroArgumentReference>());
}

void CppPreprocessor::startExpandingMacro(unsigned offset, unsigned line,
                                          const Macro &macro,
                                          const QVector<MacroArgumentReference> &actuals)
{
    if (! m_currentDoc)
        return;

    m_currentDoc->addMacroUse(revision(m_workingCopy, macro), offset, macro.name().length(), line, actuals);
}

void CppPreprocessor::stopExpandingMacro(unsigned, const Macro &)
{
    if (! m_currentDoc)
        return;

    //qDebug() << "stop expanding:" << macro.name;
}

void CppPreprocessor::markAsIncludeGuard(const QByteArray &macroName)
{
    if (!m_currentDoc)
        return;

    m_currentDoc->setIncludeGuardMacroName(macroName);
}

void CppPreprocessor::mergeEnvironment(Document::Ptr doc)
{
    if (! doc)
        return;

    const QString fn = doc->fileName();

    if (m_processed.contains(fn))
        return;

    m_processed.insert(fn);

    foreach (const Document::Include &incl, doc->includes()) {
        QString includedFile = incl.fileName();

        if (Document::Ptr includedDoc = m_snapshot.document(includedFile))
            mergeEnvironment(includedDoc);
        else
            run(includedFile);
    }

    m_env.addMacros(doc->definedMacros());
}

void CppPreprocessor::startSkippingBlocks(unsigned offset)
{
    //qDebug() << "start skipping blocks:" << offset;
    if (m_currentDoc)
        m_currentDoc->startSkippingBlocks(offset);
}

void CppPreprocessor::stopSkippingBlocks(unsigned offset)
{
    //qDebug() << "stop skipping blocks:" << offset;
    if (m_currentDoc)
        m_currentDoc->stopSkippingBlocks(offset);
}

void CppPreprocessor::sourceNeeded(unsigned line, const QString &fileName, IncludeType type)
{
    if (fileName.isEmpty())
        return;

    QString absoluteFileName = resolveFile(fileName, type);
    absoluteFileName = QDir::cleanPath(absoluteFileName);
    if (m_currentDoc && !absoluteFileName.isEmpty())
        m_currentDoc->addIncludeFile(absoluteFileName, line);
    if (m_included.contains(absoluteFileName))
        return; // we've already seen this file.
    if (absoluteFileName != modelManager()->configurationFileName())
        m_included.insert(absoluteFileName);

    unsigned editorRevision = 0;
    QString contents;
    getFileContents(absoluteFileName, &contents, &editorRevision);
    if (m_currentDoc) {
        if (contents.isEmpty() && ! QFileInfo(absoluteFileName).isAbsolute()) {
            QString msg = QCoreApplication::translate(
                    "CppPreprocessor", "%1: No such file or directory").arg(fileName);

            Document::DiagnosticMessage d(Document::DiagnosticMessage::Warning,
                                          m_currentDoc->fileName(),
                                          line, /*column = */ 0,
                                          msg);

            m_currentDoc->addDiagnosticMessage(d);

            //qWarning() << "file not found:" << fileName << m_currentDoc->fileName() << env.current_line;

            return;
        }
    }

    if (m_dumpFileNameWhileParsing) {
        qDebug() << "Parsing file:" << absoluteFileName
//             << "contents:" << contents.size()
                    ;
    }

    Document::Ptr doc = m_snapshot.document(absoluteFileName);
    if (doc) {
        mergeEnvironment(doc);
        return;
    }

    doc = Document::create(absoluteFileName);
    doc->setRevision(m_revision);
    doc->setEditorRevision(editorRevision);

    QFileInfo info(absoluteFileName);
    if (info.exists())
        doc->setLastModified(info.lastModified());

    Document::Ptr previousDoc = switchDocument(doc);

    const QByteArray preprocessedCode = m_preprocess.run(absoluteFileName, contents);

//    { QByteArray b(preprocessedCode); b.replace("\n", "<<<\n"); qDebug("Preprocessed code for \"%s\": [[%s]]", fileName.toUtf8().constData(), b.constData()); }

    doc->setUtf8Source(preprocessedCode);
    doc->keepSourceAndAST();
    doc->tokenize();

    m_snapshot.insert(doc);
    m_todo.remove(absoluteFileName);

    Process process(m_modelManager, doc, m_workingCopy);
    process();

    (void) switchDocument(previousDoc);
}

Document::Ptr CppPreprocessor::switchDocument(Document::Ptr doc)
{
    Document::Ptr previousDoc = m_currentDoc;
    m_currentDoc = doc;
    return previousDoc;
}
