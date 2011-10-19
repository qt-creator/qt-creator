/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljswrapinloader.h"
#include "qmljsquickfixassist.h"

#include <coreplugin/ifile.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsutils.h>
#include <qmljs/qmljsbind.h>
#include <qmljstools/qmljsrefactoringchanges.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;

namespace {

class Operation: public QmlJSQuickFixOperation
{
    UiObjectDefinition *m_objDef;

public:
    Operation(const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
              UiObjectDefinition *objDef)
        : QmlJSQuickFixOperation(interface, 0)
        , m_objDef(objDef)
    {
        Q_ASSERT(m_objDef != 0);

        setDescription(WrapInLoader::tr("Wrap Component in Loader"));
    }

    QString findFreeName(const QString &base)
    {
        QString tryName = base;
        int extraNumber = 1;
        const ObjectValue *found = 0;
        const ScopeChain &scope = assistInterface()->semanticInfo().scopeChain();
        forever {
            scope.lookup(tryName, &found);
            if (!found || extraNumber > 1000)
                break;
            tryName = base + QString::number(extraNumber++);
        }
        return tryName;
    }

    virtual void performChanges(QmlJSRefactoringFilePtr currentFile,
                                const QmlJSRefactoringChanges &)
    {
        UiScriptBinding *idBinding;
        const QString id = idOfObject(m_objDef, &idBinding);
        QString baseName = id;
        if (baseName.isEmpty()) {
            for (UiQualifiedId *it = m_objDef->qualifiedTypeNameId; it; it = it->next) {
                if (!it->next)
                    baseName = it->name.toString();
            }
        }

        // find ids
        const QString componentId = findFreeName(QLatin1String("component_") + baseName);
        const QString loaderId = findFreeName(QLatin1String("loader_") + baseName);

        Utils::ChangeSet changes;
        int objDefStart = m_objDef->firstSourceLocation().begin();
        int objDefEnd = m_objDef->lastSourceLocation().end();
        QString comment = WrapInLoader::tr(
                    "// TODO: Move position bindings from the component to the Loader.\n"
                    "//       Check all uses of 'parent' inside the root element of the component.\n");
        if (idBinding) {
            comment += WrapInLoader::tr(
                        "//       Rename all outer uses of the id '%1' to '%2.item'.\n").arg(
                        id, loaderId);
        }
        changes.insert(objDefStart, comment +
                       QString("Component {\n"
                               "    id: %1\n").arg(componentId));
        changes.insert(objDefEnd, QString("\n"
                                          "}\n"
                                          "Loader {\n"
                                          "    id: %2\n"
                                          "    sourceComponent: %1\n"
                                          "}\n").arg(componentId, loaderId));
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(Range(objDefStart, objDefEnd));
        currentFile->apply();
    }
};

} // end of anonymous namespace


QList<QmlJSQuickFixOperation::Ptr> WrapInLoader::match(
    const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface)
{
    const int pos = interface->currentFile()->cursor().position();

    QList<Node *> path = interface->semanticInfo().rangePath(pos);
    for (int i = path.size() - 1; i >= 0; --i) {
        Node *node = path.at(i);
        if (UiObjectDefinition *objDef = cast<UiObjectDefinition *>(node)) {
            if (!interface->currentFile()->isCursorOn(objDef->qualifiedTypeNameId))
                return noResult();
             // check that the node is not the root node
            if (i > 0 && !cast<UiProgram*>(path.at(i - 1))) {
                return singleResult(new Operation(interface, objDef));
            }
        }
    }

    return noResult();
}
