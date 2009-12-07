/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppmodelmanagerinterface.h"
#include "cpptoolsconstants.h"
#include "cppfilesettingspage.h"

#include <cplusplus/Overview.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <CoreTypes.h>
#include <Names.h>
#include <Symbols.h>
#include <Scope.h>

#include <coreplugin/icore.h>

namespace CppTools {

AbstractEditorSupport::AbstractEditorSupport(CppModelManagerInterface *modelmanager) :
    m_modelmanager(modelmanager)
{
}

AbstractEditorSupport::~AbstractEditorSupport()
{
}

void AbstractEditorSupport::updateDocument()
{
    m_modelmanager->updateSourceFiles(QStringList(fileName()));
}

QString AbstractEditorSupport::functionAt(const CppModelManagerInterface *modelManager,
                                          const QString &fileName,
                                          int line, int column)
{
    const CPlusPlus::Snapshot snapshot = modelManager->snapshot();
    const CPlusPlus::Document::Ptr document = snapshot.document(fileName);
    if (!document)
        return QString();
    if (const CPlusPlus::Symbol *symbol = document->findSymbolAt(line, column))
        if (const CPlusPlus::Scope *scope = symbol->scope())
            if (const CPlusPlus::Scope *functionScope = scope->enclosingFunctionScope())
                if (const CPlusPlus::Symbol *function = functionScope->owner()) {
                    const CPlusPlus::Overview o;
                    return o.prettyName(function->name());
                }
    return QString();
}

QString AbstractEditorSupport::licenseTemplate()
{
    return Internal::CppFileSettings::licenseTemplate();
}
}

