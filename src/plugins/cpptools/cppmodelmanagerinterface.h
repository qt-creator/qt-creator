/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
#include <projectexplorer/toolchain.h>

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QStringList>
#include <QFuture>

namespace Core { class IEditor; }
namespace CPlusPlus { class LookupContext; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class BaseTextEditor; }
namespace Utils { class FileName; }

namespace CppTools {
class AbstractEditorSupport;
class CppCompletionSupport;
class CppCompletionAssistProvider;
class CppEditorSupport;
class CppHighlightingSupport;
class CppHighlightingSupportFactory;
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
        OpenMP = 0x8
    };
    Q_DECLARE_FLAGS(CXXExtensions, CXXExtension)

    enum QtVersion {
        UnknownQt = -1,
        NoQt = 0,
        Qt4 = 1,
        Qt5 = 2
    };

    typedef QSharedPointer<ProjectPart> Ptr;

public: //attributes
    QList<ProjectFile> files;
    QByteArray defines;
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

class CPPTOOLS_EXPORT CppModelManagerInterface : public QObject
{
    Q_OBJECT

public:

    class CPPTOOLS_EXPORT ProjectInfo
    {
    public:
        ProjectInfo()
        { }

        ProjectInfo(QPointer<ProjectExplorer::Project> project)
            : m_project(project)
        { }

        operator bool() const
        { return ! m_project.isNull(); }

        bool isValid() const
        { return ! m_project.isNull(); }

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

    private: // attributes
        QPointer<ProjectExplorer::Project> m_project;
        QList<ProjectPart::Ptr> m_projectParts;
        // the attributes below are calculated from the project parts.
        QStringList m_includePaths;
        QStringList m_frameworkPaths;
        QStringList m_sourceFiles;
        QByteArray m_defines;
    };

    class CPPTOOLS_EXPORT WorkingCopy
    {
    public:
        void insert(const QString &fileName, const QString &source, unsigned revision = 0)
        { _elements.insert(fileName, qMakePair(source, revision)); }

        bool contains(const QString &fileName) const
        { return _elements.contains(fileName); }

        QString source(const QString &fileName) const
        { return _elements.value(fileName).first; }

        QPair<QString, unsigned> get(const QString &fileName) const
        { return _elements.value(fileName); }

        QHashIterator<QString, QPair<QString, unsigned> > iterator() const
        { return QHashIterator<QString, QPair<QString, unsigned> >(_elements); }

        int size() const
        { return _elements.size(); }

    private:
        typedef QHash<QString, QPair<QString, unsigned> > Table;
        Table _elements;
    };

public:
    static const QString configurationFileName();

public:
    CppModelManagerInterface(QObject *parent = 0);
    virtual ~CppModelManagerInterface();

    static CppModelManagerInterface *instance();

    virtual bool isCppEditor(Core::IEditor *editor) const = 0;

    virtual WorkingCopy workingCopy() const = 0;
    virtual CPlusPlus::Snapshot snapshot() const = 0;

    virtual QList<ProjectInfo> projectInfos() const = 0;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const = 0;
    virtual void updateProjectInfo(const ProjectInfo &pinfo) = 0;
    virtual QList<ProjectPart::Ptr> projectPart(const QString &fileName) const = 0;

    virtual void addEditorSupport(CppTools::AbstractEditorSupport *editorSupport) = 0;
    virtual void removeEditorSupport(CppTools::AbstractEditorSupport *editorSupport) = 0;
    virtual CppEditorSupport *cppEditorSupport(TextEditor::BaseTextEditor *editor) = 0;

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  const CPlusPlus::LookupContext &context) = 0;

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString()) = 0;
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) = 0;

    virtual void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement = QString()) = 0;
    virtual void findMacroUsages(const CPlusPlus::Macro &macro) = 0;

    virtual void setExtraDiagnostics(const QString &fileName, const QString &kind,
                                     const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics) = 0;

    virtual CppTools::CppCompletionSupport *completionSupport(Core::IEditor *editor) const = 0;
    virtual void setCppCompletionAssistProvider(CppTools::CppCompletionAssistProvider *completionAssistProvider) = 0;

    virtual CppTools::CppHighlightingSupport *highlightingSupport(Core::IEditor *editor) const = 0;
    virtual void setHighlightingSupportFactory(CppTools::CppHighlightingSupportFactory *highlightingFactory) = 0;

    virtual void setIndexingSupport(CppTools::CppIndexingSupport *indexingSupport) = 0;
    virtual CppTools::CppIndexingSupport *indexingSupport() = 0;

Q_SIGNALS:
    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void sourceFilesRefreshed(const QStringList &files);

    /// \brief Emitted after updateProjectInfo method is called on the model-manager.
    ///
    /// Other classes can use this to get notified when the \c ProjectExplorer has updated the parts.
    void projectPartsUpdated(ProjectExplorer::Project *project);

public Q_SLOTS:
    virtual void updateModifiedSourceFiles() = 0;
    virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles) = 0;
    virtual void GC() = 0;
};

} // namespace CppTools

#endif // CPPMODELMANAGERINTERFACE_H
