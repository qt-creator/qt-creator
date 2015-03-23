/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <qmljs/qmljsutils.h>
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
          m_lastSourceLocation(objDef->lastSourceLocation())
    {
        init();
    }

    Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
              UiObjectBinding *objDef)
        : QmlJSQuickFixOperation(interface, 0),
          m_idName(idOfObject(objDef)),
          m_firstSourceLocation(objDef->qualifiedTypeNameId->firstSourceLocation()),
          m_lastSourceLocation(objDef->lastSourceLocation())
    {
        init();
    }

    virtual void performChanges(QmlJSRefactoringFilePtr currentFile,
                                const QmlJSRefactoringChanges &refactoring)
    {
        QString componentName = m_componentName;
        QString path = QFileInfo(fileName()).path();
        bool confirm = ComponentNameDialog::go(&componentName, &path, Core::ICore::dialogParent());

        if (!confirm)
            return;

        if (componentName.isEmpty() || path.isEmpty())
            return;

        const QString newFileName = path + QLatin1Char('/') + componentName
                + QLatin1String(".qml");

        QString imports;
        UiProgram *prog = currentFile->qmljsDocument()->qmlProgram();
        if (prog && prog->headers) {
            const int start = currentFile->startOf(prog->headers->firstSourceLocation());
            const int end = currentFile->startOf(prog->members->member->firstSourceLocation());
            imports = currentFile->textOf(start, end);
        }

        const int start = currentFile->startOf(m_firstSourceLocation);
        const int end = currentFile->startOf(m_lastSourceLocation);
        const QString txt = imports + currentFile->textOf(start, end)
                + QLatin1String("}\n");

        // stop if we can't create the new file
        if (!refactoring.createFile(newFileName, txt))
            return;

        if (path == QFileInfo(fileName()).path()) {
            // hack for the common case, next version should use the wizard
            ProjectExplorer::Node * oldFileNode =
                    ProjectExplorer::SessionManager::nodeForFile(Utils::FileName::fromString(fileName()));
            if (oldFileNode) {
                ProjectExplorer::FolderNode *containingFolder = oldFileNode->parentFolderNode();
                if (containingFolder)
                    containingFolder->addFiles(QStringList(newFileName));
            }
        }

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
        QString replacement = componentName + QLatin1String(" {\n");
        if (!m_idName.isEmpty())
            replacement += QLatin1String("id: ") + m_idName + QLatin1Char('\n');

        Utils::ChangeSet changes;
        changes.replace(start, end, replacement);
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(Range(start, end + 1));
        currentFile->apply();
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
                result.append(new Operation(interface, objDef));
                return;
            }
        } else if (UiObjectBinding *objBinding = cast<UiObjectBinding *>(node)) {
            if (!interface->currentFile()->isCursorOn(objBinding->qualifiedTypeNameId))
                return;
            result.append(new Operation(interface, objBinding));
            return;
        }
    }
}

} //namespace QmlJSEditor
