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

#include "codemodelhelpers.h"

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <QCoreApplication>
#include <QDebug>

// Debug helpers for code model. @todo: Move to some CppTools library?

typedef QMap<QString, QStringList> DependencyMap;
typedef CPlusPlus::Document::Ptr DocumentPtr;
typedef QList<CPlusPlus::Symbol *> SymbolList;
typedef QList<DocumentPtr> DocumentPtrList;

static const char setupUiC[] = "setupUi";

// Find the generated "ui_form.h" header of the form via project.
static QString generatedHeaderOf(const QString &uiFileName)
{
    if (const ProjectExplorer::Project *uiProject =
            ProjectExplorer::SessionManager::projectForFile(Utils::FileName::fromString(uiFileName))) {
        QStringList files = uiProject->filesGeneratedFrom(uiFileName);
        if (!files.isEmpty()) // There should be at most one header generated from a .ui
            return files.front();
    }
    return QString();
}

namespace {
// Find function symbols in a document by name.
class SearchFunction : public CPlusPlus::SymbolVisitor {
public:
    typedef QList<CPlusPlus::Function *> FunctionList;

    explicit SearchFunction(const char *name);
    FunctionList operator()(const DocumentPtr &doc);

    virtual bool visit(CPlusPlus::Function * f);

private:
    const size_t m_length;
    const char *m_name;

    FunctionList m_matches;
};

SearchFunction::SearchFunction(const char *name) :
    m_length(qstrlen(name)),
    m_name(name)
{
}

SearchFunction::FunctionList SearchFunction::operator()(const DocumentPtr &doc)
{
    m_matches.clear();
    const unsigned globalSymbolCount = doc->globalSymbolCount();
    for (unsigned i = 0; i < globalSymbolCount; ++i)
        accept(doc->globalSymbolAt(i));
    return m_matches;
}

bool SearchFunction::visit(CPlusPlus::Function * f)
{
    if (const CPlusPlus::Name *name = f->name())
        if (const CPlusPlus::Identifier *id = name->identifier())
            if (id->size() == m_length)
                if (!qstrncmp(m_name, id->chars(), uint(m_length)))
                    m_matches.push_back(f);
    return true;
}

} // anonymous namespace

namespace Designer {
namespace Internal {

// Goto slot invoked by the designer context menu. Either navigates
// to an existing slot function or create a new one.
bool navigateToSlot(const QString &uiFileName,
                    const QString & /* objectName */,
                    const QString & /* signalSignature */,
                    const QStringList & /* parameterNames */,
                    QString *errorMessage)
{

    // Find the generated header.
    const QString generatedHeaderFile = generatedHeaderOf(uiFileName);
    if (generatedHeaderFile.isEmpty()) {
        *errorMessage = QCoreApplication::translate("Designer", "The generated header of the form \"%1\" could not be found.\nRebuilding the project might help.").arg(uiFileName);
        return false;
    }
    const CPlusPlus::Snapshot snapshot = CppTools::CppModelManager::instance()->snapshot();
    const DocumentPtr generatedHeaderDoc = snapshot.document(generatedHeaderFile);
    if (!generatedHeaderDoc) {
        *errorMessage = QCoreApplication::translate("Designer", "The generated header \"%1\" could not be found in the code model.\nRebuilding the project might help.").arg(generatedHeaderFile);
        return false;
    }

    // Look for setupUi
    SearchFunction searchFunc(setupUiC);
    const SearchFunction::FunctionList funcs = searchFunc(generatedHeaderDoc);
    if (funcs.size() != 1) {
        *errorMessage = QString::fromLatin1("Internal error: The function \"%1\" could not be found in %2").arg(QLatin1String(setupUiC), generatedHeaderFile);
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace Designer
