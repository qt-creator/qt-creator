// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsquickfix.h"
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

QmlJSQuickFixOperation::QmlJSQuickFixOperation(const QmlJSQuickFixAssistInterface *interface,
                                               int priority)
    : QuickFixOperation(priority)
    , m_semanticInfo(interface->semanticInfo())
{
}

void QmlJSQuickFixOperation::perform()
{
    QmlJSRefactoringChanges refactoring(ModelManagerInterface::instance(), semanticInfo().snapshot);
    QmlJSRefactoringFilePtr current = refactoring.file(fileName());

    performChanges(current, refactoring);
}

const QmlJSTools::SemanticInfo &QmlJSQuickFixOperation::semanticInfo() const
{
    return m_semanticInfo;
}

Utils::FilePath QmlJSQuickFixOperation::fileName() const
{
    return semanticInfo().document->fileName();
}

} // namespace QmlJSEditor
