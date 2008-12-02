/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include "formeditorplugin.h"
#include "workbenchintegration.h"
#include "formeditorw.h"
#include "formwindoweditor.h"

#include <cpptools/cppmodelmanagerinterface.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Overview.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Name.h>
#include <cplusplus/Names.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Control.h>
#include <cplusplus/LookupContext.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/itexteditable.h>

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtCore/QFileInfo>

#include <QtCore/QDebug>

using namespace Designer::Internal;
using namespace CPlusPlus;
using namespace TextEditor;

WorkbenchIntegration::WorkbenchIntegration(QDesignerFormEditorInterface *core, FormEditorW *parent) :
    qdesigner_internal::QDesignerIntegration(core, ::qobject_cast<QObject*>(parent)),
    m_few(parent)
{
    setResourceFileWatcherBehaviour(QDesignerIntegration::ReloadSilently);
    setResourceEditingEnabled(false);
    setSlotNavigationEnabled(true);
    connect(this, SIGNAL(navigateToSlot(QString, QString)),
            this, SLOT(slotNavigateToSlot(QString, QString)));
}

void WorkbenchIntegration::updateSelection()
{
    if (FormWindowEditor *afww = m_few->activeFormWindow())
        afww->updateFormWindowSelectionHandles(true);
    qdesigner_internal::QDesignerIntegration::updateSelection();
}

QWidget *WorkbenchIntegration::containerWindow(QWidget * /*widget*/) const
{
    FormWindowEditor *fw = m_few->activeFormWindow();
    if (!fw)
        return 0;
    return fw->integrationContainer();
}

QList<Document::Ptr> WorkbenchIntegration::findDocuments(const QString &uiFileName) const
{
    Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    CppTools::CppModelManagerInterface *cppModelManager =
        core->pluginManager()->getObject<CppTools::CppModelManagerInterface>();

    QList<Document::Ptr> docList;
    // take all docs
    CppTools::CppModelManagerInterface::DocumentTable docTable = cppModelManager->documents();
    foreach (Document::Ptr doc, docTable) { // we go through all documents
        QStringList includes = doc->includedFiles();
        foreach (QString include, includes) {
            const QFileInfo fi(include); // TODO: we should match an absolute path. Currently uiFileName is a file name only.
            if (fi.fileName() == uiFileName) { // we are only interested in docs which includes our ui file
                docList.append(doc);
            }
        }
    }
    return docList;
}



Class *WorkbenchIntegration::findClass(Namespace *parentNameSpace, const QString &uiClassName) const
{
    for (unsigned i = 0; i < parentNameSpace->memberCount(); i++) { // we go through all namespace members
        if (Class *cl = parentNameSpace->memberAt(i)->asClass()) { // we have found a class - we are interested in classes only
            Overview o;
            QString className = o.prettyName(cl->name());
            for (unsigned j = 0; j < cl->memberCount(); j++) { // we go through class members
                const Declaration *decl = cl->memberAt(j)->asDeclaration();
                if (decl) { // we want to know if the class contains a member (so we look into a declaration) of uiClassName type
                    const QString v1 = QLatin1String("Ui::") + uiClassName; // TODO: handle also the case of namespaced class name
                    const QString v2 = QLatin1String("Ui_") + uiClassName;

                    NamedType *nt = decl->type()->asNamedType();

                    // handle pointers to member variables
                    if (PointerType *pt = decl->type()->asPointerType())
                        nt = pt->elementType()->asNamedType();

                    if (nt) {
                        Overview typeOverview;
                        const QString memberClass = typeOverview.prettyName(nt->name());
                        if (memberClass == v1 || memberClass == v2) // simple match here
                            return cl;
                    }
                }
            }
        } else if (Namespace *ns = parentNameSpace->memberAt(i)->asNamespace()) {
            return findClass(ns, uiClassName);
        }
    }
    return 0;
}

Function *WorkbenchIntegration::findFunction(Class *cl, const QString &functionName) const
{
    // TODO: match properly function name and argument list, for now we match only function name
    // (Roberto's suggestion: use QtMethodAST for that, enclosing function with SIGNAL(<fun>)
    // then it becames an expression). Robetro's also proposed he can add public methods to parse declarations

    // Quick implementation start
    QString funName = functionName.left(functionName.indexOf(QLatin1Char('(')));
    // Quick implementation end
    for (unsigned j = 0; j < cl->memberCount(); j++) { // go through all members
        const Declaration *decl = cl->memberAt(j)->asDeclaration();
        if (decl) { // we are interested only in declarations (can be decl of method or of a field)
            Function *fun = decl->type()->asFunction();
            if (fun) { // we are only interested in declarations of methods
                Overview typeOverview;
                const QString memberFunction = typeOverview.prettyName(fun->name());
                if (memberFunction == funName) // simple match (we match only fun name, we should match also arguments)
                    return fun;
            }
        }
    }
    return 0;
}

// TODO: remove me, see below
static bool isCompatible(Name *name, Name *otherName)
{
    if (NameId *nameId = name->asNameId()) {
        if (TemplateNameId *otherTemplId = otherName->asTemplateNameId())
            return nameId->identifier()->isEqualTo(otherTemplId->identifier());
    } else if (TemplateNameId *templId = name->asTemplateNameId()) {
        if (NameId *otherNameId = otherName->asNameId())
            return templId->identifier()->isEqualTo(otherNameId->identifier());
    }

    return name->isEqualTo(otherName);
}

// TODO: remove me, see below
static bool isCompatible(Function *definition, Symbol *declaration, QualifiedNameId *declarationName)
{
    Function *declTy = declaration->type()->asFunction();
    if (! declTy)
        return false;

    Name *definitionName = definition->name();
    if (QualifiedNameId *q = definitionName->asQualifiedNameId()) {
        if (! isCompatible(q->unqualifiedNameId(), declaration->name()))
            return false;
        else if (q->nameCount() > declarationName->nameCount())
            return false;
        else if (declTy->argumentCount() != definition->argumentCount())
            return false;
        else if (declTy->isConst() != definition->isConst())
            return false;
        else if (declTy->isVolatile() != definition->isVolatile())
            return false;

        for (unsigned i = 0; i < definition->argumentCount(); ++i) {
            Symbol *arg = definition->argumentAt(i);
            Symbol *otherArg = declTy->argumentAt(i);
            if (! arg->type().isEqualTo(otherArg->type()))
                return false;
        }

        for (unsigned i = 0; i != q->nameCount(); ++i) {
            Name *n = q->nameAt(q->nameCount() - i - 1);
            Name *m = declarationName->nameAt(declarationName->nameCount() - i - 1);
            if (! isCompatible(n, m))
                return false;
        }
        return true;
    } else {
        // ### TODO: implement isCompatible for unqualified name ids.
    }
    return false;
}

// TODO: remove me, this is taken from cppeditor.cpp. Find some common place for this method
Document::Ptr WorkbenchIntegration::findDefinition(Function *functionDeclaration, int *line) const
{
    Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    CppTools::CppModelManagerInterface *cppModelManager =
        core->pluginManager()->getObject<CppTools::CppModelManagerInterface>();
    if (!cppModelManager)
        return Document::Ptr();

    QVector<Name *> qualifiedName;
    Scope *scope = functionDeclaration->scope();
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->isClassScope() || scope->isNamespaceScope()) {
            if (scope->owner() && scope->owner()->name()) {
                Name *scopeOwnerName = scope->owner()->name();
                if (QualifiedNameId *q = scopeOwnerName->asQualifiedNameId()) {
                    for (unsigned i = 0; i < q->nameCount(); ++i) {
                        qualifiedName.prepend(q->nameAt(i));
                    }
                } else {
                    qualifiedName.prepend(scopeOwnerName);
                }
            }
        }
    }

    qualifiedName.append(functionDeclaration->name());

    Control control;
    QualifiedNameId *q = control.qualifiedNameId(&qualifiedName[0], qualifiedName.size());
    LookupContext context(&control);

    const QMap<QString, Document::Ptr> documents = cppModelManager->documents();
    foreach (Document::Ptr doc, documents) {
        QList<Scope *> visibleScopes;
        visibleScopes.append(doc->globalSymbols());
        visibleScopes = context.expand(visibleScopes);
        foreach (Scope *visibleScope, visibleScopes) {
            Symbol *symbol = 0;
            if (NameId *nameId = q->unqualifiedNameId()->asNameId())
                symbol = visibleScope->lookat(nameId->identifier());
            else if (DestructorNameId *dtorId = q->unqualifiedNameId()->asDestructorNameId())
                symbol = visibleScope->lookat(dtorId->identifier());
            else if (TemplateNameId *templNameId = q->unqualifiedNameId()->asTemplateNameId())
                symbol = visibleScope->lookat(templNameId->identifier());
            else if (OperatorNameId *opId = q->unqualifiedNameId()->asOperatorNameId())
                symbol = visibleScope->lookat(opId->kind());
            // ### cast operators
            for (; symbol; symbol = symbol->next()) {
                if (! symbol->isFunction())
                    continue;
                else if (! isCompatible(symbol->asFunction(), functionDeclaration, q))
                    continue;
                *line = symbol->line(); // TODO: shift the line so that we are inside a function. Maybe just find the nearest '{'?
                return doc;
            }
        }
    }
    return Document::Ptr();

}

void WorkbenchIntegration::addDeclaration(const QString &docFileName, Class *cl, const QString &functionName) const
{
    // TODO: add argument names (from designer we get only argument types)
    for (unsigned j = 0; j < cl->memberCount(); j++) { // go through all members
        const Declaration *decl = cl->memberAt(j)->asDeclaration();
        if (decl) { // we want to find any method which is a private slot (then we don't need to add "private slots:" statement)
            Function *fun = decl->type()->asFunction();
            if (fun) { // we are only interested in declarations of methods
                if (fun->isSlot() && fun->isPrivate()) {
                    ITextEditable *editable = qobject_cast<ITextEditable *>(
                                TextEditor::BaseTextEditor::openEditorAt(docFileName, fun->line()/*, fun->column()*/)); // TODO: fun->column() gives me weird number...
                    if (editable) {
                        editable->insert(QLatin1String("void ") + functionName + QLatin1String(";\n    "));
                    }
                    return;
                }
            }
        }
    }

    // TODO: we didn't find "private slots:", let's add it

    return;
}

void WorkbenchIntegration::slotNavigateToSlot(const QString &objectName, const QString &signalSignature)
{
    const QString currentUiFile = m_few->activeFormWindow()->file()->fileName();

    // TODO: we should pass to findDocuments an absolute path to generated .h file from ui.
    // Currently we are guessing the name of ui_<>.h file and pass the file name only to the findDocuments().
    // The idea is that the .pro file knows if the .ui files is inside, and the .pro file knows it will
    // be generating the ui_<>.h file for it, and the .pro file knows what the generated file's name and its absolute path will be.
    // So we should somehow get that info from project manager (?)
    const QFileInfo fi(currentUiFile);
    const QString uicedName = QLatin1String("ui_") + fi.baseName() + QLatin1String(".h");

    QList<Document::Ptr> docList = findDocuments(uicedName);
    if (docList.isEmpty())
        return;

    QDesignerFormWindowInterface *fwi = m_few->activeFormWindow()->formWindow();

    const QString uiClassName = fwi->mainContainer()->objectName();

    foreach (Document::Ptr doc, docList) {
        Class *cl = findClass(doc->globalNamespace(), uiClassName);
        if (cl) {
            QString functionName = QLatin1String("on_") + objectName + QLatin1Char('_') + signalSignature;
            Function *fun = findFunction(cl, functionName);
            int line = 0;
            if (!fun) {
                // add function declaration to cl
                addDeclaration(doc->fileName(), cl, functionName);
                // TODO: add function definition to cpp file

            } else {
                doc = findDefinition(fun, &line);
            }
            if (doc) {
                // jump to function definition
                TextEditor::BaseTextEditor::openEditorAt(doc->fileName(), line);
            }
            return;
        }
    }
}

