/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CPPMODELMANAGERINTERFACE_H
#define CPPMODELMANAGERINTERFACE_H

#include <cplusplus/CppDocument.h>
#include <languageutils/fakemetaobject.h>
#include "cpptools_global.h"

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QStringList>
#include <QFuture>

namespace Core {
    class IEditor;
}

namespace CPlusPlus {
    class LookupContext;
}

namespace ProjectExplorer {
    class Project;
}

namespace CppTools {
    class AbstractEditorSupport;
    class CppCompletionSupport;
    class CppCompletionAssistProvider;
    class CppHighlightingSupport;
    class CppHighlightingSupportFactory;
}

namespace CPlusPlus {

class CPPTOOLS_EXPORT CppModelManagerInterface : public QObject
{
    Q_OBJECT

public:
    enum Language { CXX, OBJC };

    class CPPTOOLS_EXPORT ProjectPart
    {
    public:
        ProjectPart()
            : language(CXX)
            , cxx11Enabled(false)
            , qtVersion(UnknownQt)
        {}

    public: //attributes
        QStringList sourceFiles;
        QByteArray defines;
        QStringList includePaths;
        QStringList frameworkPaths;
        QStringList precompiledHeaders;
        Language language;
        bool cxx11Enabled;
        enum QtVersion {
            UnknownQt = -1,
            NoQt = 0,
            Qt4 = 1,
            Qt5 = 2
        };
        QtVersion qtVersion;

        bool objcEnabled() const
        { return language == CppModelManagerInterface::OBJC; }

        typedef QSharedPointer<ProjectPart> Ptr;
    };

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

    private:
        typedef QHash<QString, QPair<QString, unsigned> > Table;
        Table _elements;
    };

    enum ExtraDiagnosticKind
    {
        AllExtraDiagnostics = -1,
        ExportedQmlTypesDiagnostic,
        CppSemanticsDiagnostic
    };

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

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  const CPlusPlus::LookupContext &context) = 0;

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString()) = 0;
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) = 0;

    virtual void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement = QString()) = 0;
    virtual void findMacroUsages(const CPlusPlus::Macro &macro) = 0;

    virtual void setExtraDiagnostics(const QString &fileName, int key,
                                     const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics) = 0;
    virtual QList<CPlusPlus::Document::DiagnosticMessage> extraDiagnostics(
            const QString &fileName, int key = AllExtraDiagnostics) const = 0;

    virtual CppTools::CppCompletionSupport *completionSupport(Core::IEditor *editor) const = 0;
    virtual void setCppCompletionAssistProvider(CppTools::CppCompletionAssistProvider *completionAssistProvider) = 0;

    virtual CppTools::CppHighlightingSupport *highlightingSupport(Core::IEditor *editor) const = 0;
    virtual void setHighlightingSupportFactory(CppTools::CppHighlightingSupportFactory *highlightingFactory) = 0;

Q_SIGNALS:
    void documentUpdated(CPlusPlus::Document::Ptr doc);
    void sourceFilesRefreshed(const QStringList &files);
    void extraDiagnosticsUpdated(QString fileName);

public Q_SLOTS:
    virtual void updateModifiedSourceFiles() = 0;
    virtual QFuture<void> updateSourceFiles(const QStringList &sourceFiles) = 0;
    virtual void GC() = 0;
};

} // namespace CPlusPlus

#endif // CPPMODELMANAGERINTERFACE_H
