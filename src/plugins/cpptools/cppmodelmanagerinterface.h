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

#include "cppprojects.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/cppmodelmanagerbase.h>

#include <QFuture>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>

namespace Core { class IEditor; }
namespace CPlusPlus { class LookupContext; }
namespace ProjectExplorer { class Project; }
namespace TextEditor { class BaseTextEditor; class BaseTextDocument; class BlockRange; }
namespace Utils { class FileName; }

namespace CppTools {

class AbstractEditorSupport;
class BaseEditorDocumentProcessor;
class CppCompletionAssistProvider;
class EditorDocumentHandle;
class CppIndexingSupport;
class ModelManagerSupport;
class WorkingCopy;

class CPPTOOLS_EXPORT CppModelManagerInterface : public CPlusPlus::CppModelManagerBase
{
    Q_OBJECT

public:
     // Documented in source file.
     enum ProgressNotificationMode {
        ForcedProgressNotification,
        ReservedProgressNotification
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

    virtual EditorDocumentHandle *editorDocument(const QString &filePath) = 0;
    virtual void registerEditorDocument(EditorDocumentHandle *editorDocument) = 0;
    virtual void unregisterEditorDocument(const QString &filePath) = 0;

    virtual QList<int> references(CPlusPlus::Symbol *symbol,
                                  const CPlusPlus::LookupContext &context) = 0;

    virtual void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                              const QString &replacement = QString()) = 0;
    virtual void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) = 0;

    virtual void renameMacroUsages(const CPlusPlus::Macro &macro, const QString &replacement = QString()) = 0;
    virtual void findMacroUsages(const CPlusPlus::Macro &macro) = 0;

    virtual void finishedRefreshingSourceFiles(const QStringList &files) = 0;

    virtual void addModelManagerSupport(ModelManagerSupport *modelManagerSupport) = 0;
    virtual ModelManagerSupport *modelManagerSupportForMimeType(const QString &mimeType) const = 0;
    virtual CppCompletionAssistProvider *completionAssistProvider(const QString &mimeType) const = 0;
    virtual BaseEditorDocumentProcessor *editorDocumentProcessor(
                TextEditor::BaseTextDocument *baseTextDocument) const = 0;

    virtual void setIndexingSupport(CppTools::CppIndexingSupport *indexingSupport) = 0;
    virtual CppIndexingSupport *indexingSupport() = 0;

    virtual void setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths) = 0;
    virtual void enableGarbageCollector(bool enable) = 0;

    virtual ProjectPart::HeaderPaths headerPaths() = 0;
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
};

} // namespace CppTools

#endif // CPPMODELMANAGERINTERFACE_H
