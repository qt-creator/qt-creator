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
    connect(this, SIGNAL(navigateToSlot(QString, QString, QStringList)),
            this, SLOT(slotNavigateToSlot(QString, QString, QStringList)));
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

static QList<Document::Ptr> findDocumentsIncluding(const QString &fileName, bool checkFileNameOnly)
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
            if (checkFileNameOnly) {
                const QFileInfo fi(include);
                if (fi.fileName() == fileName) { // we are only interested in docs which includes fileName only
                    docList.append(doc);
                }
            } else {
                if (include == fileName)
                    docList.append(doc);
            }
        }
    }
    return docList;
}

static Class *findClass(Namespace *parentNameSpace, const QString &uiClassName, QString *namespaceName)
{
    // construct proper ui class name, take into account namespaced ui class name
    QString className1;
    QString className2;
    int indexOfScope = uiClassName.lastIndexOf(QLatin1String("::"));
    if (indexOfScope < 0) {
        className1 = QLatin1String("Ui::") + uiClassName;
        className2 = QLatin1String("Ui_") + uiClassName;
    } else {
        className1 = uiClassName.left(indexOfScope + 2) + QLatin1String("Ui::") + uiClassName.mid(indexOfScope + 2);
        className2 = uiClassName.left(indexOfScope + 2) + QLatin1String("Ui_") + uiClassName.mid(indexOfScope + 2);
    }

    for (unsigned i = 0; i < parentNameSpace->memberCount(); i++) { // we go through all namespace members
        if (Class *cl = parentNameSpace->memberAt(i)->asClass()) { // we have found a class - we are interested in classes only
            Overview o;
            QString className = o.prettyName(cl->name());
            for (unsigned j = 0; j < cl->memberCount(); j++) { // we go through class members
                const Declaration *decl = cl->memberAt(j)->asDeclaration();
                if (decl) { // we want to know if the class contains a member (so we look into a declaration) of uiClassName type
                    NamedType *nt = decl->type()->asNamedType();

                    // handle pointers to member variables
                    if (PointerType *pt = decl->type()->asPointerType())
                        nt = pt->elementType()->asNamedType();

                    if (nt) {
                        Overview typeOverview;
                        const QString memberClass = typeOverview.prettyName(nt->name());
                        if (memberClass == className1 || memberClass == className2) // names match
                            return cl;
                        // memberClass can be shorter (can have some namespaces cut because of e.g. "using namespace" declaration)
                        if (memberClass == className1.right(memberClass.length())) { // memberClass lenght <= className length
                            const QString namespacePrefix = className1.left(className1.length() - memberClass.length());
                            if (namespacePrefix.right(2) == QLatin1String("::"))
                                return cl;
                        }
                        // the same as above but for className2
                        if (memberClass == className2.right(memberClass.length())) { // memberClass lenght <= className length
                            const QString namespacePrefix = className2.left(className1.length() - memberClass.length());
                            if (namespacePrefix.right(2) == QLatin1String("::"))
                                return cl;
                        }
                    }
                }
            }
        } else if (Namespace *ns = parentNameSpace->memberAt(i)->asNamespace()) {
            Overview o;
            QString tempNS = *namespaceName + o.prettyName(ns->name()) + QLatin1String("::");
            Class *cl = findClass(ns, uiClassName, &tempNS);
            if (cl) {
                *namespaceName = tempNS;
                return cl;
            }
        }
    }
    return 0;
}

static Function *findDeclaration(Class *cl, const QString &functionName)
{
    const QString funName = QString::fromUtf8(QMetaObject::normalizedSignature(functionName.toUtf8()));
    for (unsigned j = 0; j < cl->memberCount(); j++) { // go through all members
        const Declaration *decl = cl->memberAt(j)->asDeclaration();
        if (decl) { // we are interested only in declarations (can be decl of method or of a field)
            Function *fun = decl->type()->asFunction();
            if (fun) { // we are only interested in declarations of methods
                Overview overview;
                QString memberFunction = overview.prettyName(fun->name()) + QLatin1Char('(');
                for (uint i = 0; i < fun->argumentCount(); i++) { // we build argument types string
                    Argument *arg = fun->argumentAt(i)->asArgument();
                    if (i > 0)
                        memberFunction += QLatin1Char(',');
                    memberFunction += overview.prettyType(arg->type());
                }
                memberFunction += QLatin1Char(')');
                // we compare normalized signatures
                memberFunction = QString::fromUtf8(QMetaObject::normalizedSignature(memberFunction.toUtf8()));
                if (memberFunction == funName) // we match function names and argument lists
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
static Document::Ptr findDefinition(Function *functionDeclaration, int *line)
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

// TODO: Wait for robust Roberto's code using AST or whatever for that. Current implementation is hackish.
static int findClassEndPosition(const QString &headerContents, int classStartPosition)
{
    const QString contents = headerContents.mid(classStartPosition); // we start serching from the beginning of class declaration
    // We need to find the position of class closing "}"
    int openedBlocksCount = 0; // counter of nested {} blocks
    int idx = 0; // index of current position in the contents
    while (true) {
        if (idx < 0 || idx >= contents.length()) // indexOf returned -1, that means we don't have closing comment mark
            break;
        if (contents.mid(idx, 2) == QLatin1String("//")) {
            idx = contents.indexOf(QLatin1Char('\n'), idx + 2) + 1; // drop everything up to the end of line
        } else if (contents.mid(idx, 2) == QLatin1String("/*")) {
            idx = contents.indexOf(QLatin1String("*/"), idx + 2) + 1; // drop everything up to the nearest */
        } else if (contents.mid(idx, 4) == QLatin1String("'\\\"'")) {
            idx += 4; // drop it
        } else if (contents.at(idx) == QLatin1Char('\"')) {
            do {
                idx = contents.indexOf(QLatin1Char('\"'), idx + 1); // drop everything up to the nearest "
            } while (idx > 0 && contents.at(idx - 1) == QLatin1Char('\\')); // if the nearest " is preceeded by \ we find next one
            if (idx < 0)
                break;
            idx++;
        } else {
            if (contents.at(idx) == QLatin1Char('{')) {
                openedBlocksCount++;
            } else if (contents.at(idx) == QLatin1Char('}')) {
                openedBlocksCount--;
                if (openedBlocksCount == 0) {
                    return classStartPosition + idx;
                }
            }
            idx++;
        }
    }
    return -1;
}

static void addDeclaration(const QString &docFileName, Class *cl, const QString &functionName)
{
    // functionName comes already with argument names (if designer managed to do that)
    for (unsigned j = 0; j < cl->memberCount(); j++) { // go through all members
        const Declaration *decl = cl->memberAt(j)->asDeclaration();
        if (decl) { // we want to find any method which is a private slot (then we don't need to add "private slots:" statement)
            Function *fun = decl->type()->asFunction();
            if (fun) { // we are only interested in declarations of methods
                if (fun->isSlot() && fun->isPrivate()) {
                    ITextEditable *editable = qobject_cast<ITextEditable *>(
                                TextEditor::BaseTextEditor::openEditorAt(docFileName, fun->line(), fun->column()));
                    // fun->column() raturns always 0, what can cause trouble in case in one
                    // line there is: "private slots: void foo();"
                    if (editable) {
                        editable->insert(QLatin1String("void ") + functionName + QLatin1String(";\n    "));
                    }
                    return;
                }
            }
        }
    }

    // We didn't find any method under "private slots:", let's add "private slots:". Below code
    // adds "private slots:" by the end of the class definition.

    ITextEditable *editable = qobject_cast<ITextEditable *>(
                       TextEditor::BaseTextEditor::openEditorAt(docFileName, cl->line(), cl->column()));
    if (editable) {
        int classEndPosition = findClassEndPosition(editable->contents(), editable->position());
        if (classEndPosition >= 0) {
            int line, column;
            editable->convertPosition(classEndPosition, &line, &column); // converts back position into a line and column
            editable->gotoLine(line, column);  // go to position (we should be just before closing } of the class)
            editable->insert(QLatin1String("\nprivate slots:\n    ")
                      + QLatin1String("void ") + functionName + QLatin1String(";\n"));
        }
    }
}

static Document::Ptr addDefinition(const QString &headerFileName, const QString &className,
                  const QString &functionName, int *line)
{
    // we find all documents which include headerFileName
    QList<Document::Ptr> docList = findDocumentsIncluding(headerFileName, false);
    if (docList.isEmpty())
        return Document::Ptr();

    QFileInfo headerFI(headerFileName);
    const QString headerBaseName = headerFI.baseName();
    const QString headerAbsolutePath = headerFI.absolutePath();
    foreach (Document::Ptr doc, docList) {
        QFileInfo sourceFI(doc->fileName());
        // we take only those documents which has the same filename and path (maybe we don't need to compare the path???)
        if (headerBaseName == sourceFI.baseName() && headerAbsolutePath == sourceFI.absolutePath()) {
            ITextEditable *editable = qobject_cast<ITextEditable *>(
                            TextEditor::BaseTextEditor::openEditorAt(doc->fileName(), 0));
            if (editable) {
                const QString contents = editable->contents();
                int column;
                editable->convertPosition(contents.length(), line, &column);
                editable->gotoLine(*line, column);
                editable->insert(QLatin1String("\nvoid ") + className + QLatin1String("::") +
                                 functionName + QLatin1String("\n  {\n\n  }\n"));
                *line += 1;

            }
            return doc;
        }
    }
    return Document::Ptr();
}

static QString addParameterNames(const QString &functionSignature, const QStringList &parameterNames)
{
    QString functionName = functionSignature.left(functionSignature.indexOf(QLatin1Char('(')) + 1);
    QString argumentsString = functionSignature.mid(functionSignature.indexOf(QLatin1Char('(')) + 1);
    argumentsString = argumentsString.left(argumentsString.indexOf(QLatin1Char(')')));
    const QStringList arguments = argumentsString.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (int i = 0; i < arguments.count(); ++i) {
        if (i > 0)
            functionName += QLatin1String(", ");
        functionName += arguments.at(i);
        if (i < parameterNames.count())
            functionName += QLatin1Char(' ') + parameterNames.at(i);
    }
    functionName += QLatin1Char(')');
    return functionName;
}

void WorkbenchIntegration::slotNavigateToSlot(const QString &objectName, const QString &signalSignature,
        const QStringList &parameterNames)
{
    const QString currentUiFile = m_few->activeFormWindow()->file()->fileName();

    // TODO: we should pass to findDocumentsIncluding an absolute path to generated .h file from ui.
    // Currently we are guessing the name of ui_<>.h file and pass the file name only to the findDocumentsIncluding().
    // The idea is that the .pro file knows if the .ui files is inside, and the .pro file knows it will
    // be generating the ui_<>.h file for it, and the .pro file knows what the generated file's name and its absolute path will be.
    // So we should somehow get that info from project manager (?)
    const QFileInfo fi(currentUiFile);
    const QString uicedName = QLatin1String("ui_") + fi.baseName() + QLatin1String(".h");

    QList<Document::Ptr> docList = findDocumentsIncluding(uicedName, true); // change to false when we know the absolute path to generated ui_<>.h file
    if (docList.isEmpty())
        return;

    QDesignerFormWindowInterface *fwi = m_few->activeFormWindow()->formWindow();

    const QString uiClassName = fwi->mainContainer()->objectName();

    foreach (Document::Ptr doc, docList) {
        QString namespaceName; // namespace of the class found
        Class *cl = findClass(doc->globalNamespace(), uiClassName, &namespaceName);
        if (cl) {
            Overview o;
            const QString className = namespaceName + o.prettyName(cl->name());

            QString functionName = QLatin1String("on_") + objectName + QLatin1Char('_') + signalSignature;
            QString functionNameWithParameterNames = addParameterNames(functionName, parameterNames);
            Function *fun = findDeclaration(cl, functionName);
            int line = 0;
            Document::Ptr sourceDoc;
            if (!fun) {
                // add function declaration to cl
                addDeclaration(doc->fileName(), cl, functionNameWithParameterNames);

                // add function definition to cpp file
                sourceDoc = addDefinition(doc->fileName(), className, functionNameWithParameterNames, &line);
            } else {
                sourceDoc = findDefinition(fun, &line);
                if (!sourceDoc) {
                    // add function definition to cpp file
                    sourceDoc = addDefinition(doc->fileName(), className, functionNameWithParameterNames, &line);
                }
            }
            if (sourceDoc) {
                // jump to function definition
                TextEditor::BaseTextEditor::openEditorAt(sourceDoc->fileName(), line);
            }
            return;
        }
    }
}

