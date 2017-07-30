/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "classviewcontroller.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <cplusplus/Overview.h>
#include <cplusplus/LookupContext.h>

namespace ModelEditor {
namespace Internal {

ClassViewController::ClassViewController(QObject *parent)
    : QObject(parent)
{
}

QSet<QString> ClassViewController::findClassDeclarations(const QString &fileName)
{
    QSet<QString> classNames;

    CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
    CPlusPlus::Snapshot snapshot = cppModelManager->snapshot();

    // scan original file
    CPlusPlus::Document::Ptr document = snapshot.document(fileName);
    if (!document.isNull())
        appendClassDeclarationsFromDocument(document, &classNames);

    QString otherFileName = CppTools::correspondingHeaderOrSource(fileName);

    // scan other file
    document = snapshot.document(otherFileName);
    if (!document.isNull())
        appendClassDeclarationsFromDocument(document, &classNames);

    return classNames;
}

void ClassViewController::appendClassDeclarationsFromDocument(CPlusPlus::Document::Ptr document,
                                                            QSet<QString> *classNames)
{
    int total = document->globalSymbolCount();
    for (int i = 0; i < total; ++i) {
        CPlusPlus::Symbol *symbol = document->globalSymbolAt(i);
        appendClassDeclarationsFromSymbol(symbol, classNames);
    }
}

void ClassViewController::appendClassDeclarationsFromSymbol(CPlusPlus::Symbol *symbol,
                                                          QSet<QString> *classNames)
{
    if (symbol->isClass()) {
        CPlusPlus::Overview overview;
        QString className = overview.prettyName(
                    CPlusPlus::LookupContext::fullyQualifiedName(symbol));
        // Ignore private class created by Q_OBJECT macro
        if (!className.endsWith("::QPrivateSignal"))
            classNames->insert(className);
    }

    if (symbol->isScope()) {
        CPlusPlus::Scope *scope = symbol->asScope();
        int total = scope->memberCount();
        for (int i = 0; i < total; ++i) {
            CPlusPlus::Symbol *member = scope->memberAt(i);
            appendClassDeclarationsFromSymbol(member, classNames);
        }
    }
}

} // namespace Internal
} // namespace ModelEditor
