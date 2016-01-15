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

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljseditor.h"
#include "qmljsquickfixassist.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsast_p.h>

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSTools;
using TextEditor::RefactoringChanges;

namespace QmlJSEditor {

using namespace Internal;

QmlJSQuickFixOperation::QmlJSQuickFixOperation(const QmlJSQuickFixInterface &interface,
                                               int priority)
    : QuickFixOperation(priority)
    , m_interface(interface)
{
}

void QmlJSQuickFixOperation::perform()
{
    QmlJSRefactoringChanges refactoring(ModelManagerInterface::instance(),
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

} // namespace QmlJSEditor
