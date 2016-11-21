/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljscomponentfromobjectdef.h"
#include "qmljscomponentnamedialog.h"
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
#include <projectexplorer/session.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>

#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

using namespace QmlJS::AST;
using namespace QmlJSTools;

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
        }

        setDescription(QCoreApplication::translate("QmlJSEditor::ComponentFromObjectDef",
                                                   "Move Component into Separate File"));
    }

    Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
              UiObjectDefinition *objDef)
        : QmlJSQuickFixOperation(interface, 0),
          m_idName(idOfObject(objDef)),
          m_firstSourceLocation(objDef->firstSourceLocation()),
          m_lastSourceLocation(objDef->lastSourceLocation()),
          m_initializer(objDef->initializer)
    {
        init();
    }

    Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
              UiObjectBinding *objDef)
        : QmlJSQuickFixOperation(interface, 0),
          m_idName(idOfObject(objDef)),
          m_firstSourceLocation(objDef->qualifiedTypeNameId->firstSourceLocation()),
          m_lastSourceLocation(objDef->lastSourceLocation()),
          m_initializer(objDef->initializer)
    {
        init();
    }

    virtual void performChanges(QmlJSRefactoringFilePtr currentFile,
                                const QmlJSRefactoringChanges &refactoring)
    {
        QString componentName = m_componentName;

        const QString currentFileName = currentFile->qmljsDocument()->fileName();
        QString path = QFileInfo(currentFileName).path();

        QmlJS::PropertyReader propertyReader(currentFile->qmljsDocument(), m_initializer);
        QStringList result;
        QStringList sourcePreview;

        QString suffix;

        if (!m_idName.isEmpty())
            sourcePreview.append(QLatin1String("    id: ") + m_idName);
        else
            sourcePreview.append(QString());

        QStringList sortedPropertiesWithoutId;

        foreach (const QString &property, propertyReader.properties())
            if (property != QLatin1String("id"))
                sortedPropertiesWithoutId.append(property);

        sortedPropertiesWithoutId.sort();

        foreach (const QString &property, sortedPropertiesWithoutId)
            sourcePreview.append(QLatin1String("    ") + property + QLatin1String(": ") + propertyReader.readAstValue(property));

        const bool confirm = ComponentNameDialog::go(&componentName, &path, &suffix,
                                               sortedPropertiesWithoutId,
                                               sourcePreview,
                                               QFileInfo(currentFileName).fileName(),
                                               &result,
                                               Core::ICore::dialogParent());
        if (!confirm)
            return;

        if (componentName.isEmpty() || path.isEmpty())
            return;

        const QString newFileName = path + QLatin1Char('/') + componentName
                + QLatin1String(".") + suffix;

        QString imports;
        UiProgram *prog = currentFile->qmljsDocument()->qmlProgram();
        if (prog && prog->headers) {
            const unsigned int start = currentFile->startOf(prog->headers->firstSourceLocation());
            const unsigned int end = currentFile->startOf(prog->members->member->firstSourceLocation());
            imports = currentFile->textOf(start, end);
        }

        const unsigned int start = currentFile->startOf(m_firstSourceLocation);
        const unsigned int end = currentFile->startOf(m_lastSourceLocation);
        QString newComponentSource = imports + currentFile->textOf(start, end)
                + QLatin1String("}\n");

        //Remove properties from resulting code...

        Utils::ChangeSet changeSet;
        QmlJS::Rewriter rewriter(newComponentSource, &changeSet, QStringList());

        QmlJS::Dialect dialect = QmlJS::Dialect::Qml;

        QmlJS::Document::MutablePtr doc = QmlJS::Document::create(newFileName, dialect);
        doc->setSource(newComponentSource);
        doc->parseQml();

        if (doc->isParsedCorrectly()) {

            UiObjectMember *astRootNode = 0;
            if (UiProgram *program = doc->qmlProgram())
                if (program->members)
                    astRootNode = program->members->member;

            foreach (const QString &property, result)
                rewriter.removeBindingByName(initializerOfObject(astRootNode), property);
        } else {
            qWarning() << Q_FUNC_INFO << "parsing failed:" << newComponentSource;
        }

        changeSet.apply(&newComponentSource);

        // stop if we can't create the new file
        const bool reindent = true;
        const bool openEditor = false;
        if (!refactoring.createFile(newFileName, newComponentSource, reindent, openEditor))
            return;

        if (path == QFileInfo(currentFileName).path()) {
            // hack for the common case, next version should use the wizard
            ProjectExplorer::Node * oldFileNode =
                    ProjectExplorer::SessionManager::nodeForFile(Utils::FileName::fromString(currentFileName));
            if (oldFileNode) {
                ProjectExplorer::FolderNode *containingFolder = oldFileNode->parentFolderNode();
                if (containingFolder)
                    containingFolder->addFiles(QStringList(newFileName));
            }
        }

        QString replacement = componentName + QLatin1String(" {\n");
        if (!m_idName.isEmpty())
            replacement += QLatin1String("id: ") + m_idName + QLatin1Char('\n');

        foreach (const QString &property, result)
            replacement += property + QLatin1String(": ") + propertyReader.readAstValue(property) + QLatin1Char('\n');

        Utils::ChangeSet changes;
        changes.replace(start, end, replacement);
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(Range(start, end + 1));
        currentFile->apply();

        Core::IVersionControl *versionControl = Core::VcsManager::findVersionControlForDirectory(path);
        if (versionControl
                && versionControl->supportsOperation(Core::IVersionControl::AddOperation)) {
            const QMessageBox::StandardButton button =
                    QMessageBox::question(Core::ICore::mainWindow(),
                                          Core::VcsManager::msgAddToVcsTitle(),
                                          Core::VcsManager::msgPromptToAddToVcs(QStringList(newFileName), versionControl),
                                          QMessageBox::Yes | QMessageBox::No);
            if (button == QMessageBox::Yes && !versionControl->vcsAdd(newFileName)) {
                QMessageBox::warning(Core::ICore::mainWindow(),
                                     Core::VcsManager::msgAddToVcsFailedTitle(),
                                     Core::VcsManager::msgToAddToVcsFailed(QStringList(newFileName), versionControl));
            }
        }
    }
};

} // end of anonymous namespace


void ComponentFromObjectDef::match(const QmlJSQuickFixInterface &interface, QuickFixOperations &result)
{
    const int pos = interface->currentFile()->cursor().position();

    QList<Node *> path = interface->semanticInfo().rangePath(pos);
    for (int i = path.size() - 1; i >= 0; --i) {
        Node *node = path.at(i);
        if (UiObjectDefinition *objDef = cast<UiObjectDefinition *>(node)) {

            if (!interface->currentFile()->isCursorOn(objDef->qualifiedTypeNameId))
                return;
             // check that the node is not the root node
            if (i > 0 && !cast<UiProgram*>(path.at(i - 1))) {
                result << new Operation(interface, objDef);
                return;
            }
        } else if (UiObjectBinding *objBinding = cast<UiObjectBinding *>(node)) {
            if (!interface->currentFile()->isCursorOn(objBinding->qualifiedTypeNameId))
                return;
            result << new Operation(interface, objBinding);
            return;
        }
    }
}

void ComponentFromObjectDef::perform(const QString &fileName, QmlJS::AST::UiObjectDefinition *objDef)
{
    QmlJSRefactoringChanges refactoring(QmlJS::ModelManagerInterface::instance(),
                                        QmlJS::ModelManagerInterface::instance()->snapshot());
    QmlJSRefactoringFilePtr current = refactoring.file(fileName);

    QmlJSQuickFixInterface interface;
    Operation operation(interface, objDef);

    operation.performChanges(current, refactoring);
}

} //namespace QmlJSEditor
