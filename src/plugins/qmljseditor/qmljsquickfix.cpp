// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsquickfix.h"
#include "qmljscomponentfromobjectdef.h"
#include "qmljseditor.h"
#include "qmljsquickfixassist.h"

#include <extensionsystem/iplugin.h>
#include <extensionsystem/pluginmanager.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/parser/qmljsast_p.h>

#include <utils/algorithm.h>

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

Utils::FilePath QmlJSQuickFixOperation::fileName() const
{
    return m_interface->semanticInfo().document->fileName();
}

} // namespace QmlJSEditor
