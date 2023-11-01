// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewcontroller.h"

#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cplusplus/Overview.h>
#include <cplusplus/LookupContext.h>

using namespace Utils;
namespace ModelEditor {
namespace Internal {

ClassViewController::ClassViewController(QObject *parent)
    : QObject(parent)
{
}

QSet<QString> ClassViewController::findClassDeclarations(const FilePath &filePath, int line, int column)
{
    QSet<QString> classNames;

    CPlusPlus::Snapshot snapshot = CppEditor::CppModelManager::snapshot();

    // scan original file
    CPlusPlus::Document::Ptr document = snapshot.document(filePath);
    if (!document.isNull())
        appendClassDeclarationsFromDocument(document, line, column, &classNames);

    if (line <= 0) {
        const FilePath otherFilePath = CppEditor::correspondingHeaderOrSource(filePath);

        // scan other file
        document = snapshot.document(otherFilePath);
        if (!document.isNull())
            appendClassDeclarationsFromDocument(document, -1, -1, &classNames);
    }
    return classNames;
}

void ClassViewController::appendClassDeclarationsFromDocument(CPlusPlus::Document::Ptr document,
                                                              int line, int column,
                                                              QSet<QString> *classNames)
{
    int total = document->globalSymbolCount();
    for (int i = 0; i < total; ++i) {
        CPlusPlus::Symbol *symbol = document->globalSymbolAt(i);
        appendClassDeclarationsFromSymbol(symbol, line, column, classNames);
    }
}

void ClassViewController::appendClassDeclarationsFromSymbol(CPlusPlus::Symbol *symbol,
                                                            int line, int column,
                                                            QSet<QString> *classNames)
{
    if (symbol->asClass()
            && (line <= 0 || (symbol->line() == line && symbol->column() == column + 1)))
    {
        CPlusPlus::Overview overview;
        QString className = overview.prettyName(
                    CPlusPlus::LookupContext::fullyQualifiedName(symbol));
        // Ignore private class created by Q_OBJECT macro
        if (!className.endsWith("::QPrivateSignal"))
            classNames->insert(className);
    }

    if (symbol->asScope()) {
        CPlusPlus::Scope *scope = symbol->asScope();
        int total = scope->memberCount();
        for (int i = 0; i < total; ++i) {
            CPlusPlus::Symbol *member = scope->memberAt(i);
            appendClassDeclarationsFromSymbol(member, line, column, classNames);
        }
    }
}

} // namespace Internal
} // namespace ModelEditor
