/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "clangcompletion.h"
#include "clanghighlightingsupport.h"
#include "clangmodelmanagersupport.h"

#include <QCoreApplication>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

ModelManagerSupport::ModelManagerSupport(FastIndexer *fastIndexer)
    : m_completionAssistProvider(new ClangCompletionAssistProvider)
    , m_fastIndexer(fastIndexer)
{
}

ModelManagerSupport::~ModelManagerSupport()
{
}

QString ModelManagerSupport::id() const
{
    return QLatin1String("ClangCodeMode.ClangCodeMode");
}

QString ModelManagerSupport::displayName() const
{
    //: Display name
    return QCoreApplication::translate("ClangCodeModel::Internal::ModelManagerSupport",
                                       "Clang");
}

CppTools::CppCompletionAssistProvider *ModelManagerSupport::completionAssistProvider()
{
    return m_completionAssistProvider.data();
}

CppTools::CppHighlightingSupport *ModelManagerSupport::highlightingSupport(
        TextEditor::BaseTextDocument *baseTextDocument)
{
    return new ClangHighlightingSupport(baseTextDocument, m_fastIndexer);
}
