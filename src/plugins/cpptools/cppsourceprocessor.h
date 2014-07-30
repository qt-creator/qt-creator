#ifndef CPPSOURCEPROCESSOR_H
#define CPPSOURCEPROCESSOR_H

#include "cppmodelmanagerinterface.h"
#include "cppworkingcopy.h"

#include <cplusplus/PreprocessorEnvironment.h>
#include <cplusplus/pp-engine.h>
#include <utils/qtcoverride.h>

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QStringList>

#include <functional>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace CppTools {
namespace Internal {

// Documentation inside.
class CppSourceProcessor: public CPlusPlus::Client
{
    Q_DISABLE_COPY(CppSourceProcessor)

public:
    typedef std::function<void (const CPlusPlus::Document::Ptr &)> DocumentCallback;

public:
    static QString cleanPath(const QString &path);

    CppSourceProcessor(const CPlusPlus::Snapshot &snapshot, DocumentCallback documentFinished);
    ~CppSourceProcessor();

    void setDumpFileNameWhileParsing(bool onoff)
    { m_dumpFileNameWhileParsing = onoff; }

    void setRevision(unsigned revision);
    void setWorkingCopy(const CppTools::WorkingCopy &workingCopy);
    void setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths);
    void setTodo(const QStringList &files);

    void run(const QString &fileName, const QStringList &initialIncludes = QStringList());
    void removeFromCache(const QString &fileName);
    void resetEnvironment();

    CPlusPlus::Snapshot snapshot() const { return m_snapshot; }
    const QSet<QString> &todo() const { return m_todo; }

    void setGlobalSnapshot(const CPlusPlus::Snapshot &snapshot) { m_globalSnapshot = snapshot; }

private:
    void addFrameworkPath(const ProjectPart::HeaderPath &frameworkPath);

    CPlusPlus::Document::Ptr switchCurrentDocument(CPlusPlus::Document::Ptr doc);

    bool getFileContents(const QString &absoluteFilePath, QByteArray *contents,
                         unsigned *revision) const;
    bool checkFile(const QString &absoluteFilePath) const;
    QString resolveFile(const QString &fileName, IncludeType type);
    QString resolveFile_helper(const QString &fileName, IncludeType type);

    void mergeEnvironment(CPlusPlus::Document::Ptr doc);

    // Client interface
    void macroAdded(const CPlusPlus::Macro &macro) QTC_OVERRIDE;
    void passedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                    unsigned line, const CPlusPlus::Macro &macro) QTC_OVERRIDE;
    void failedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charOffset,
                                    const CPlusPlus::ByteArrayRef &name) QTC_OVERRIDE;
    void notifyMacroReference(unsigned bytesOffset, unsigned utf16charOffset,
                              unsigned line, const CPlusPlus::Macro &macro) QTC_OVERRIDE;
    void startExpandingMacro(unsigned bytesOffset, unsigned utf16charOffset,
                             unsigned line, const CPlusPlus::Macro &macro,
                             const QVector<CPlusPlus::MacroArgumentReference> &actuals) QTC_OVERRIDE;
    void stopExpandingMacro(unsigned bytesOffset, const CPlusPlus::Macro &macro) QTC_OVERRIDE;
    void markAsIncludeGuard(const QByteArray &macroName) QTC_OVERRIDE;
    void startSkippingBlocks(unsigned utf16charsOffset) QTC_OVERRIDE;
    void stopSkippingBlocks(unsigned utf16charsOffset) QTC_OVERRIDE;
    void sourceNeeded(unsigned line, const QString &fileName, IncludeType type,
                      const QStringList &initialIncludes) QTC_OVERRIDE;

private:
    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::Snapshot m_globalSnapshot;
    DocumentCallback m_documentFinished;
    bool m_dumpFileNameWhileParsing;
    CPlusPlus::Environment m_env;
    CPlusPlus::Preprocessor m_preprocess;
    ProjectPart::HeaderPaths m_headerPaths;
    CppTools::WorkingCopy m_workingCopy;
    QSet<QString> m_included;
    CPlusPlus::Document::Ptr m_currentDoc;
    QSet<QString> m_todo;
    QSet<QString> m_processed;
    unsigned m_revision;
    QHash<QString, QString> m_fileNameCache;
    QTextCodec *m_defaultCodec;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPSOURCEPROCESSOR_H
