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

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljseditor.h"
#include "qmljs/parser/qmljsast_p.h"
#include "qmljsquickfixassist.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QDebug>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSTools;
using namespace TextEditor;
using TextEditor::RefactoringChanges;

QmlJSQuickFixOperation::QmlJSQuickFixOperation(
        const QSharedPointer<const QmlJSQuickFixAssistInterface> &interface,
        int priority)
    : QuickFixOperation(priority)
    , m_interface(interface)
{
}

QmlJSQuickFixOperation::~QmlJSQuickFixOperation()
{
}

void QmlJSQuickFixOperation::perform()
{
    QmlJSRefactoringChanges refactoring(QmlJS::ModelManagerInterface::instance(),
                                        m_interface->semanticInfo().snapshot);
    QmlJSRefactoringFilePtr current = refactoring.file(fileName());

    performChanges(current, refactoring);
}

const QmlJSQuickFixAssistInterface *QmlJSQuickFixOperation::assistInterface() const
{
    return m_interface.data();
}

QString QmlJSQuickFixOperation::fileName() const
{
    return m_interface->semanticInfo().document->fileName();
}

QmlJSQuickFixFactory::QmlJSQuickFixFactory()
{
}

QmlJSQuickFixFactory::~QmlJSQuickFixFactory()
{
}

QList<QuickFixOperation::Ptr> QmlJSQuickFixFactory::matchingOperations(
    const QSharedPointer<const TextEditor::IAssistInterface> &interface)
{
    return match(interface.staticCast<const QmlJSQuickFixAssistInterface>());
}

QList<QmlJSQuickFixOperation::Ptr> QmlJSQuickFixFactory::noResult()
{
    return QList<QmlJSQuickFixOperation::Ptr>();
}

QList<QmlJSQuickFixOperation::Ptr> QmlJSQuickFixFactory::singleResult(QmlJSQuickFixOperation *operation)
{
    QList<QmlJSQuickFixOperation::Ptr> result;
    result.append(QmlJSQuickFixOperation::Ptr(operation));
    return result;
}
