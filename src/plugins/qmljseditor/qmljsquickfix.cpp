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
using TextEditor::RefactoringChanges;

QmlJSQuickFixOperation::QmlJSQuickFixOperation(const QmlJSQuickFixInterface &interface,
                                               int priority)
    : QuickFixOperation(priority)
    , m_interface(interface)
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


void QmlJSQuickFixFactory::matchingOperations(const QuickFixInterface &interface,
    QuickFixOperations &result)
{
    match(interface.staticCast<const QmlJSQuickFixAssistInterface>(), result);
}
