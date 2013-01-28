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

#include "abstracteditorsupport.h"
#include "cpptoolsconstants.h"
#include "cppfilesettingspage.h"

#include <cplusplus/Overview.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include "ModelManagerInterface.h"
#include <CoreTypes.h>
#include <Names.h>
#include <Symbols.h>
#include <Scope.h>

#include <coreplugin/icore.h>
#include <QDebug>

using namespace CPlusPlus;

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
    if (const CPlusPlus::Symbol *symbol = document->lastVisibleSymbolAt(line, column))
        if (const CPlusPlus::Scope *scope = symbol->enclosingScope())
            if (const CPlusPlus::Scope *functionScope = scope->enclosingFunction())
                if (const CPlusPlus::Symbol *function = functionScope) {
                    const CPlusPlus::Overview o;
                    QString rc = o.prettyName(function->name());
                    // Prepend namespace "Foo::Foo::foo()" up to empty root namespace
                    for (const CPlusPlus::Symbol *owner = function->enclosingNamespace();
                         owner; owner = owner->enclosingNamespace()) {
                        const QString name = o.prettyName(owner->name());
                        if (name.isEmpty()) {
                            break;
                        } else {
                            rc.prepend(QLatin1String("::"));
                            rc.prepend(name);
                        }
                    }
                    return rc;
                }
    return QString();
}

QString AbstractEditorSupport::licenseTemplate(const QString &file, const QString &className)
{
    return Internal::CppFileSettings::licenseTemplate(file, className);
}
}

