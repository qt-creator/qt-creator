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

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;

namespace {

class Operation: public QmlJSQuickFixOperation
{
    UiObjectDefinition *m_objDef;
    QString m_idName, m_componentName;

public:
    Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
              UiObjectDefinition *objDef)
        : QmlJSQuickFixOperation(interface, 0)
        , m_objDef(objDef)
    {
        Q_ASSERT(m_objDef != 0);

        m_idName = idOfObject(m_objDef);
        if (!m_idName.isEmpty()) {
            m_componentName = m_idName;
            m_componentName[0] = m_componentName.at(0).toUpper();
        }

        setDescription(QCoreApplication::translate("QmlJSEditor::ComponentFromObjectDef",
                                                   "Move Component into Separate File"));
    }

    virtual void performChanges(QmlJSRefactoringFilePtr currentFile,
                                const QmlJSRefactoringChanges &refactoring)
    {
        QString componentName = m_componentName;
        QString path = QFileInfo(fileName()).path();
        ComponentNameDialog::go(&componentName, &path, assistInterface()->editor());

        if (componentName.isEmpty() || path.isEmpty())
            return;

        const QString newFileName = path + QDir::separator() + componentName
                + QLatin1String(".qml");

        QString imports;
        UiProgram *prog = currentFile->qmljsDocument()->qmlProgram();
        if (prog && prog->imports) {
            const int start = currentFile->startOf(prog->imports->firstSourceLocation());
            const int end = currentFile->startOf(prog->members->member->firstSourceLocation());
            imports = currentFile->textOf(start, end);
        }

        const int start = currentFile->startOf(m_objDef->firstSourceLocation());
        const int end = currentFile->startOf(m_objDef->lastSourceLocation());
        const QString txt = imports + currentFile->textOf(start, end)
                + QLatin1String("}\n");

        // stop if we can't create the new file
        if (!refactoring.createFile(newFileName, txt))
            return;

        Core::IVersionControl *versionControl = Core::ICore::vcsManager()->findVersionControlForDirectory(path);
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
                result.append(QuickFixOperation::Ptr(new Operation(interface, objDef)));
                return;
            }
        }
    }
}
