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

#include "cppcompletionassist.h"
#include "cppmodelmanagersupportinternal.h"
#include "cppfollowsymbolundercursor.h"
#include "builtineditordocumentprocessor.h"

#include <app/app_version.h>

#include <QCoreApplication>

using namespace CppTools;
using namespace CppTools::Internal;

QString ModelManagerSupportProviderInternal::id() const
{
    return QLatin1String("CppTools.BuiltinCodeModel");
}

QString ModelManagerSupportProviderInternal::displayName() const
{
    return QCoreApplication::translate("ModelManagerSupportInternal::displayName",
                                       "%1 Built-in").arg(Core::Constants::IDE_DISPLAY_NAME);
}

ModelManagerSupport::Ptr ModelManagerSupportProviderInternal::createModelManagerSupport()
{
    return ModelManagerSupport::Ptr(new ModelManagerSupportInternal);
}

ModelManagerSupportInternal::ModelManagerSupportInternal()
    : m_completionAssistProvider(new InternalCompletionAssistProvider),
      m_followSymbol(new FollowSymbolUnderCursor)
{
}

ModelManagerSupportInternal::~ModelManagerSupportInternal()
{
}

BaseEditorDocumentProcessor *ModelManagerSupportInternal::editorDocumentProcessor(
        TextEditor::TextDocument *baseTextDocument)
{
    return new BuiltinEditorDocumentProcessor(baseTextDocument);
}

CppCompletionAssistProvider *ModelManagerSupportInternal::completionAssistProvider()
{
    return m_completionAssistProvider.data();
}

FollowSymbolInterface &ModelManagerSupportInternal::followSymbolInterface()
{
    return *m_followSymbol;
}
