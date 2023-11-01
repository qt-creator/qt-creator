// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codemodelhelpers.h"
#include "designertr.h"

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <QCoreApplication>
#include <QDebug>

// Debug helpers for code model. @todo: Move to some CppEditor library?

using namespace ProjectExplorer;
using namespace Utils;

using DependencyMap = QMap<QString, QStringList>;
using DocumentPtr = CPlusPlus::Document::Ptr;
using SymbolList = QList<CPlusPlus::Symbol *>;
using DocumentPtrList = QList<DocumentPtr>;

static const char setupUiC[] = "setupUi";

// Find the generated "ui_form.h" header of the form via project.
static FilePath generatedHeaderOf(const FilePath &uiFileName)
{
    if (const Project *uiProject = ProjectManager::projectForFile(uiFileName)) {
        if (Target *t = uiProject->activeTarget()) {
            if (BuildSystem *bs = t->buildSystem()) {
                FilePaths files = bs->filesGeneratedFrom(uiFileName);
                if (!files.isEmpty()) // There should be at most one header generated from a .ui
                    return files.front();
            }
        }
    }
    return {};
}

namespace {
// Find function symbols in a document by name.
class SearchFunction : public CPlusPlus::SymbolVisitor {
public:
    typedef QList<CPlusPlus::Function *> FunctionList;

    explicit SearchFunction(const char *name);
    FunctionList operator()(const DocumentPtr &doc);

    bool visit(CPlusPlus::Function * f) override;

private:
    const uint m_length;
    const char *m_name;

    FunctionList m_matches;
};

SearchFunction::SearchFunction(const char *name) :
    m_length(uint(qstrlen(name))),
    m_name(name)
{
}

SearchFunction::FunctionList SearchFunction::operator()(const DocumentPtr &doc)
{
    m_matches.clear();
    const int globalSymbolCount = doc->globalSymbolCount();
    for (int i = 0; i < globalSymbolCount; ++i)
        accept(doc->globalSymbolAt(i));
    return m_matches;
}

bool SearchFunction::visit(CPlusPlus::Function * f)
{
    if (const CPlusPlus::Name *name = f->name())
        if (const CPlusPlus::Identifier *id = name->identifier())
            if (static_cast<uint>(id->size()) == m_length)
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
    const FilePath generatedHeaderFile = generatedHeaderOf(FilePath::fromString(uiFileName));
    if (generatedHeaderFile.isEmpty()) {
        *errorMessage = Tr::tr("The generated header of the form \"%1\" could not be found.\nRebuilding the project might help.").arg(uiFileName);
        return false;
    }
    const CPlusPlus::Snapshot snapshot = CppEditor::CppModelManager::snapshot();
    const DocumentPtr generatedHeaderDoc = snapshot.document(generatedHeaderFile);
    if (!generatedHeaderDoc) {
        *errorMessage = Tr::tr("The generated header \"%1\" could not be found in the code model.\nRebuilding the project might help.").arg(generatedHeaderFile.toUserOutput());
        return false;
    }

    // Look for setupUi
    SearchFunction searchFunc(setupUiC);
    const SearchFunction::FunctionList funcs = searchFunc(generatedHeaderDoc);
    if (funcs.size() != 1) {
        *errorMessage = QString::fromLatin1(
                            "Internal error: The function \"%1\" could not be found in %2")
                            .arg(QLatin1String(setupUiC), generatedHeaderFile.toUserOutput());
        return false;
    }
    return true;
}

} // namespace Internal
} // namespace Designer
