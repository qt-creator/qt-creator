// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscomponentfromobjectdef.h"
#include "qmljscomponentnamedialog.h"
#include "qmljseditortr.h"
#include "qmljsquickfixassist.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsutils.h>
#include <qmljs/qmljspropertyreader.h>
#include <qmljs/qmljsrewriter.h>
#include <qmljstools/qmljsrefactoringchanges.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

using namespace QmlJS::AST;
using QmlJS::SourceLocation;
using namespace QmlJSTools;
using namespace Utils;

namespace QmlJSEditor {

using namespace Internal;

namespace {

class Operation: public QmlJSQuickFixOperation
{
    QString m_idName, m_componentName;
    SourceLocation m_firstSourceLocation;
    SourceLocation m_lastSourceLocation;
    UiObjectInitializer *m_initializer;
public:
    void init()
    {
        if (!m_idName.isEmpty()) {
            m_componentName = m_idName;
            m_componentName[0] = m_componentName.at(0).toUpper();
            m_componentName.prepend("My");
        }

        setDescription(Tr::tr("Move Component into Separate File"));
    }

    Operation(const Internal::QmlJSQuickFixAssistInterface *interface, UiObjectDefinition *objDef)
        : QmlJSQuickFixOperation(interface, 0)
        , m_idName(idOfObject(objDef))
        , m_firstSourceLocation(objDef->firstSourceLocation())
        , m_lastSourceLocation(objDef->lastSourceLocation())
        , m_initializer(objDef->initializer)
    {
        init();
    }

    Operation(const Internal::QmlJSQuickFixAssistInterface *interface, UiObjectBinding *objDef)
        : QmlJSQuickFixOperation(interface, 0)
        , m_idName(idOfObject(objDef))
        , m_firstSourceLocation(objDef->qualifiedTypeNameId->firstSourceLocation())
        , m_lastSourceLocation(objDef->lastSourceLocation())
        , m_initializer(objDef->initializer)
    {
        init();
    }

    void performChanges(QmlJSRefactoringFilePtr currentFile,
                        const QmlJSRefactoringChanges &refactoring,
                        const QString &imports = QString()) override
    {
        QString componentName = m_componentName;

        const Utils::FilePath currentFileName = currentFile->qmljsDocument()->fileName();
        Utils::FilePath path = currentFileName.parentDir();
        QString pathStr = path.toUserOutput();

        QmlJS::PropertyReader propertyReader(currentFile->qmljsDocument(), m_initializer);
        QStringList result;
        QStringList sourcePreview;

        QString suffix;

        if (!m_idName.isEmpty())
            sourcePreview.append(QLatin1String("    id: ") + m_idName);
        else
            sourcePreview.append(QString());

        QStringList sortedPropertiesWithoutId;

        const QStringList properties = propertyReader.properties();
        for (const QString &property : properties)
            if (property != QLatin1String("id"))
                sortedPropertiesWithoutId.append(property);

        sortedPropertiesWithoutId.sort();

        for (const QString &property : std::as_const(sortedPropertiesWithoutId))
            sourcePreview.append(QLatin1String("    ") + property + QLatin1String(": ") + propertyReader.readAstValue(property));

        const bool confirm = ComponentNameDialog::go(&componentName,
                                                     &pathStr,
                                                     &suffix,
                                                     sortedPropertiesWithoutId,
                                                     sourcePreview,
                                                     currentFileName.fileName(),
                                                     &result,
                                                     Core::ICore::dialogParent());
        if (!confirm)
            return;

        path = Utils::FilePath::fromUserInput(pathStr);
        if (componentName.isEmpty() || path.isEmpty())
            return;

        const Utils::FilePath newFileName = path.pathAppended(componentName + QLatin1String(".")
                                                              + suffix);

        QString qmlImports = imports.size() ? imports : currentFile->qmlImports();

        const unsigned int start = currentFile->startOf(m_firstSourceLocation);
        const unsigned int end = currentFile->startOf(m_lastSourceLocation);
        QString newComponentSource = qmlImports + currentFile->textOf(start, end)
                                     + QLatin1String("}\n");

        //Remove properties from resulting code...

        Utils::ChangeSet changeSet;
        QmlJS::Rewriter rewriter(newComponentSource, &changeSet, QStringList());

        QmlJS::Dialect dialect = QmlJS::Dialect::Qml;

        QmlJS::Document::MutablePtr doc = QmlJS::Document::create(newFileName, dialect);
        doc->setSource(newComponentSource);
        doc->parseQml();

        if (doc->isParsedCorrectly()) {

            UiObjectMember *astRootNode = nullptr;
            if (UiProgram *program = doc->qmlProgram())
                if (program->members)
                    astRootNode = program->members->member;

            for (const QString &property : std::as_const(result))
                rewriter.removeBindingByName(initializerOfObject(astRootNode), property);
        } else {
            qWarning() << Q_FUNC_INFO << "parsing failed:" << newComponentSource;
        }

        changeSet.apply(&newComponentSource);

        // stop if we can't create the new file
        const bool reindent = true;
        const bool openEditor = false;
        const Utils::FilePath newFilePath = newFileName;
        if (!refactoring.createFile(newFileName, newComponentSource, reindent, openEditor))
            return;

        if (path.toString() == currentFileName.toFileInfo().path()) {
            // hack for the common case, next version should use the wizard
            ProjectExplorer::Node *oldFileNode = ProjectExplorer::ProjectTree::nodeForFile(
                currentFileName);
            if (oldFileNode) {
                ProjectExplorer::FolderNode *containingFolder = oldFileNode->parentFolderNode();
                if (containingFolder)
                    containingFolder->addFiles({newFileName});
            }
        }

        QString replacement = componentName + QLatin1String(" {\n");
        if (!m_idName.isEmpty())
            replacement += QLatin1String("id: ") + m_idName + QLatin1Char('\n');

        for (const QString &property : std::as_const(result))
            replacement += property + QLatin1String(": ") + propertyReader.readAstValue(property) + QLatin1Char('\n');

        Utils::ChangeSet changes;
        changes.replace(start, end, replacement);
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(Range(start, end + 1));
        currentFile->apply();

        Core::IVersionControl *versionControl = Core::VcsManager::findVersionControlForDirectory(
            path);
        if (versionControl
                && versionControl->supportsOperation(Core::IVersionControl::AddOperation)) {
            const QMessageBox::StandardButton button = QMessageBox::question(
                Core::ICore::dialogParent(),
                Core::VcsManager::msgAddToVcsTitle(),
                Core::VcsManager::msgPromptToAddToVcs(QStringList(newFileName.toString()),
                                                      versionControl),
                QMessageBox::Yes | QMessageBox::No);
            if (button == QMessageBox::Yes && !versionControl->vcsAdd(newFileName)) {
                QMessageBox::warning(Core::ICore::dialogParent(),
                                     Core::VcsManager::msgAddToVcsFailedTitle(),
                                     Core::VcsManager::msgToAddToVcsFailed(
                                         QStringList(newFileName.toString()), versionControl));
            }
        }
    }
};

} // end of anonymous namespace


void matchComponentFromObjectDefQuickFix(const QmlJSQuickFixAssistInterface *interface, QuickFixOperations &result)
{
    const int pos = interface->currentFile()->cursor().position();

    QList<Node *> path = interface->semanticInfo().rangePath(pos);
    for (int i = path.size() - 1; i >= 0; --i) {
        Node *node = path.at(i);
        if (auto objDef = cast<UiObjectDefinition *>(node)) {

            if (!interface->currentFile()->isCursorOn(objDef->qualifiedTypeNameId))
                return;
             // check that the node is not the root node
            if (i > 0 && !cast<UiProgram*>(path.at(i - 1))) {
                result << new Operation(interface, objDef);
                return;
            }
        } else if (auto objBinding = cast<UiObjectBinding *>(node)) {
            if (!interface->currentFile()->isCursorOn(objBinding->qualifiedTypeNameId))
                return;
            result << new Operation(interface, objBinding);
            return;
        }
    }
}

void performComponentFromObjectDef(QmlJSEditorWidget *editor,
                                   const QString &fileName,
                                   QmlJS::AST::UiObjectDefinition *objDef,
                                   const QString &importData)
{
    QmlJSRefactoringChanges refactoring(QmlJS::ModelManagerInterface::instance(),
                                        QmlJS::ModelManagerInterface::instance()->snapshot());
    QmlJSRefactoringFilePtr current = refactoring.file(Utils::FilePath::fromString(fileName));

    QmlJSQuickFixAssistInterface interface(editor, TextEditor::AssistReason::ExplicitlyInvoked);
    Operation operation(&interface, objDef);

    operation.performChanges(current, refactoring, importData);
}

} //namespace QmlJSEditor
