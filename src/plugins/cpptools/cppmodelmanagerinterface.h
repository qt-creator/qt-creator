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

#ifndef CPPMODELMANAGERINTERFACE_H
#define CPPMODELMANAGERINTERFACE_H

#include "cpptools_global.h"

#include "cppprojectfile.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/cppmodelmanagerbase.h>
#include <projectexplorer/toolchain.h>

#include <QFuture>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>

namespace Core { class IEditor; }
namespace CPlusPlus { class LookupContext; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class BaseTextEditor; class BlockRange; }
namespace Utils { class FileName; }

namespace CppTools {

class AbstractEditorSupport;
class ModelManagerSupport;
class CppCompletionAssistProvider;
class CppEditorSupport;
class CppHighlightingSupport;
class CppIndexingSupport;

class CPPTOOLS_EXPORT ProjectPart
{
public:
    ProjectPart();

    void evaluateToolchain(const ProjectExplorer::ToolChain *tc,
                           const QStringList &cxxflags,
                           const QStringList &cflags,
                           const Utils::FileName &sysRoot);

public:
    enum CVersion {
        C89,
        C99,
        C11
    };

    enum CXXVersion {
        CXX98,
        CXX11
    };

    enum CXXExtension {
        NoExtensions = 0x0,
        GnuExtensions = 0x1,
        MicrosoftExtensions = 0x2,
        BorlandExtensions = 0x4,
        OpenMPExtensions = 0x8,

        AllExtensions = GnuExtensions | MicrosoftExtensions | BorlandExtensions | OpenMPExtensions
    };
    Q_DECLARE_FLAGS(CXXExtensions, CXXExtension)

    enum QtVersion {
        UnknownQt = -1,
        NoQt = 0,
        Qt4 = 1,
        Qt5 = 2
    };

    typedef QSharedPointer<ProjectPart> Ptr;

public:
    QString displayName;
    QString projectFile;
    ProjectExplorer::Project *project;
    QList<ProjectFile> files;
    QString projectConfigFile; // currently only used by the Generic Project Manager
    QByteArray projectDefines;
    QByteArray toolchainDefines;
    QStringList includePaths;
    QStringList frameworkPaths;
    QStringList precompiledHeaders;
    CVersion cVersion;
    CXXVersion cxxVersion;
    CXXExtensions cxxExtensions;
    QtVersion qtVersion;
    ProjectExplorer::ToolChain::WarningFlags cWarningFlags;
    ProjectExplorer::ToolChain::WarningFlags cxxWarningFlags;
};

class CPPTOOLS_EXPORT CppModelManagerInterface : public CPlusPlus::CppModelManagerBase
{
    Q_OBJECT

public:
     // Documented in source file.
     enum ProgressNotificationMode {
        ForcedProgressNotification,
        ReservedProgressNotification
    };

    class CPPTOOLS_EXPORT ProjectInfo
    {
    public:
        ProjectInfo()
        {}

        ProjectInfo(QPointer<ProjectExplorer::Project> project)
            : m_project(project)
        {}

        operator bool() const
        { return !m_project.isNull(); }

        bool isValid() const
        { return !m_project.isNull(); }

        bool isNull() const
        { return m_project.isNull(); }

        QPointer<ProjectExplorer::Project> project() const
        { return m_project; }

        const QList<ProjectPart::Ptr> projectParts() const
        { return m_projectParts; }

        void clearProjectParts();
        void appendProjectPart(const ProjectPart::Ptr &part);

        const QStringList includePaths() const
        { return m_includePaths; }

        const QStringList frameworkPaths() const
        { return m_frameworkPaths; }

        const QStringList sourceFiles() const
        { return m_sourceFiles; }

        const QByteArray defines() const
        { return m_defines; }

    private:
        QPointer<ProjectExplorer::Project> m_project;
        QList<ProjectPart::Ptr> m_projectParts;
        // The members below are (re)calculated from the project parts once a part is appended.
        QStringList m_includePaths;
        QStringList m_frameworkPaths;
        QStringList m_sourceFiles;
        QByteArray m_defines;
    };

    /// The working-copy stores all files that are stored on disk in their current state.
    ///
    /// So, currently the working copy holds:
    ///  - unsaved content of editors
    ///  - uic-ed UI files (through \c AbstractEditorSupport)
    ///  - the preprocessor configuration
    ///
    /// Contents are keyed on filename, and hold the revision in the editor and the editor's
    /// contents encoded as UTF-8.
    class CPPTOOLS_EXPORT WorkingCopy
    {
    public:
        void insert(const QString &fileName, const QByteArray &source, unsigned revision = 0)
        { _elements.insert(fileName, qMakePair(source, revision)); }

        bool contains(const QString &fileName) const
        { return _elements.contains(fileName); }

        QByteArray source(const QString &fileName) const
        { return _elements.value(fileName).first; }

        QPair<QByteArray, unsigned> get(const QString &fileName) const
        { return _elements.value(fileName); }

        QHashIterator<QString, QPair<QByteArray, unsigned> > iterator() const
        { return QHashIterator<QString, QPair<QByteArray, unsigned> >(_elements); }

        int size() const
        { return _elements.size(); }

    private:
        typedef QHash<QString, QPair<QByteArray, unsigned> > Table;
        Table _elements;
    };

public:
    static const QString configurationFileName();
    static const QString editorConfigurationFileName();

public:
    CppModelManagerInterface(QObject *parent = 0);
    virtual ~CppModelManagerInterface();

    static CppModelManagerInterface *instance();

    virtual bool isCppEditor(Core::IEditor *editor) const = 0;

    virtual WorkingCopy workingCopy() const = 0;
    virtual QByteArray codeModelConfiguration() const = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual QFuture<void> updateProjectInfo(const ProjectInfo &pinfo) = 0;
    virtual ProjectPart::Ptr projectPartForProjectFile(const QString &projectFile) const = 0;
    virtual QList<ProjectPart::Ptr> projectPart(const QString &fileName) const = 0;
    virtual QList<ProjectPart::Ptr> projectPartFromDependencies(const QString &fileName) const = 0;
    virtual ProjectPart::Ptr fallbackProjectPart() const = 0;

    virtual void addExtraEditorSupport(CppTools::AbstractEditorSupport *editorSupport) = 0;
    virtual void removeExtraEditorSupport(CppTools::AbstractEditorSupport *editorSupport) = 0;
    virtual CppEditorSupport *cppEditorSupport(TextEditor::BaseTextEditor *textEditor) = 0;
    virtual void deleteCppEditorSupport(TextEditor::BaseTextEditor *textEditor) = 0;

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  const CPlusPlus::LookupContext &context) = 0;

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString()) = 0;
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) = 0;

    virtual void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement = QString()) = 0;
    virtual void findMacroUsages(const CPlusPlus::Macro &macro) = 0;

    virtual void setIfdefedOutBlocks(const QString &fileName,
                                     const QList<TextEditor::BlockRange> &ifdeffedOutBlocks) = 0;

    virtual void addModelManagerSupport(ModelManagerSupport *modelManagerSupport) = 0;
    virtual ModelManagerSupport *modelManagerSupportForMimeType(const QString &mimeType) const = 0;
    virtual CppCompletionAssistProvider *completionAssistProvider(Core::IEditor *editor) const = 0;
    virtual CppHighlightingSupport *highlightingSupport(Core::IEditor *editor) const = 0;

    virtual void setIndexingSupport(CppTools::CppIndexingSupport *indexingSupport) = 0;
    virtual CppIndexingSupport *indexingSupport() = 0;

    virtual void setIncludePaths(const QStringList &includePaths) = 0;
    virtual void enableGarbageCollector(bool enable) = 0;

    virtual QStringList includePaths() = 0;
    virtual QStringList frameworkPaths() = 0;
    virtual QByteArray definedMacros() = 0;

signals:
    /// Project data might be locked while this is emitted.
    void aboutToRemoveFiles(const QStringList &files);

    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void sourceFilesRefreshed(const QStringList &files);

    /// \brief Emitted after updateProjectInfo function is called on the model-manager.
    ///
    /// Other classes can use this to get notified when the \c ProjectExplorer has updated the parts.
    void projectPartsUpdated(ProjectExplorer::Project *project);

    void globalSnapshotChanged();

public slots:
    // Documented in source file.
    virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles,
        ProgressNotificationMode mode = ReservedProgressNotification) = 0;

    virtual void updateModifiedSourceFiles() = 0;
    virtual void GC() = 0;

protected:
    static QByteArray readProjectConfigFile(const ProjectPart::Ptr &part);
};

} // namespace CppTools

#endif // CPPMODELMANAGERINTERFACE_H
