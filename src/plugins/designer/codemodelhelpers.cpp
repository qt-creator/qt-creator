/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "codemodelhelpers.h"

#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Name.h>
#include <cplusplus/Names.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Control.h>
#include <SymbolVisitor.h>

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>

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
    const ProjectExplorer::SessionManager *sessionMgr = ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    if (const ProjectExplorer::Project *uiProject = sessionMgr->projectForFile(uiFileName))
        return uiProject->generatedUiHeader(uiFileName);
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
                if (!qstrncmp(m_name, id->chars(), m_length))
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
        *errorMessage = QCoreApplication::translate("Designer", "The generated header of the form '%1' could not be found.\nRebuilding the project might help.").arg(uiFileName);
        return false;
    }
    const CPlusPlus::Snapshot snapshot = CPlusPlus::CppModelManagerInterface::instance()->snapshot();
    const DocumentPtr generatedHeaderDoc = snapshot.document(generatedHeaderFile);
    if (!generatedHeaderDoc) {
        *errorMessage = QCoreApplication::translate("Designer", "The generated header '%1' could not be found in the code model.\nRebuilding the project might help.").arg(generatedHeaderFile);
        return false;
    }

    // Look for setupUi
    SearchFunction searchFunc(setupUiC);
    const SearchFunction::FunctionList funcs = searchFunc(generatedHeaderDoc);
    if (funcs.size() != 1) {
        *errorMessage = QString::fromLatin1("Internal error: The function '%1' could not be found in in %2").arg(QLatin1String(setupUiC), generatedHeaderFile);
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace Designer
