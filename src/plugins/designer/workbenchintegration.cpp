/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/itexteditable.h>

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtGui/QMessageBox>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

enum { indentation = 4 };

using namespace Designer::Internal;
using namespace CPlusPlus;
using namespace TextEditor;

static QString msgClassNotFound(const QString &uiClassName, const QList<Document::Ptr> &docList)
{
    QString files;
    foreach (const Document::Ptr &doc, docList) {
        if (!files.isEmpty())
            files += QLatin1String(", ");
        files += doc->fileName();
    }
    return WorkbenchIntegration::tr("The class definition of '%1' could not be found in %2.").arg(uiClassName, files);
}

static inline CppTools::CppModelManagerInterface *cppModelManagerInstance()
{
    return ExtensionSystem::PluginManager::instance()
        ->getObject<CppTools::CppModelManagerInterface>();
}

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

static QList<Document::Ptr> findDocumentsIncluding(const CPlusPlus::Snapshot &docTable,
                                                   const QString &fileName, bool checkFileNameOnly)
{
    QList<Document::Ptr> docList;
    foreach (const Document::Ptr &doc, docTable) { // we go through all documents
        const QStringList includes = doc->includedFiles();
        foreach (const QString &include, includes) {
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

// Check for a class name where haystack is a member class of an object.
// So, haystack can be shorter (can have some namespaces omitted because of a
// "using namespace" declaration, for example, comparing
// "foo::Ui::form", against "using namespace foo; Ui::form".

static bool matchMemberClassName(const QString &needle, const QString &hayStack)
{
    if (needle == hayStack)
        return true;
    if (!needle.endsWith(hayStack))
        return false;
    // Check if there really is a separator "::"
    const int separatorPos = needle.size() - hayStack.size() - 1;
    return separatorPos > 1 && needle.at(separatorPos) == QLatin1Char(':');
}

// Find class definition in namespace
static const Class *findClass(const Namespace *parentNameSpace, const QString &className, QString *namespaceName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << className;

    const Overview o;
    const unsigned namespaceMemberCount = parentNameSpace->memberCount();
    for (unsigned i = 0; i < namespaceMemberCount; i++) { // we go through all namespace members
        const Symbol *sym = parentNameSpace->memberAt(i);
        // we have found a class - we are interested in classes only
        if (const Class *cl = sym->asClass()) {
            const unsigned classMemberCount = cl->memberCount();
            for (unsigned j = 0; j < classMemberCount; j++) // we go through class members
                if (const Declaration *decl = cl->memberAt(j)->asDeclaration()) {
                // we want to know if the class contains a member (so we look into
                // a declaration) of uiClassName type
                    const NamedType *nt = decl->type()->asNamedType();
                    // handle pointers to member variables
                    if (PointerType *pt = decl->type()->asPointerType())
                        nt = pt->elementType()->asNamedType();

                    if (nt && matchMemberClassName(className, o.prettyName(nt->name())))
                            return cl;
                } // decl
        } else {
            // Check namespaces
            if (const Namespace *ns = sym->asNamespace()) {
                QString tempNS = *namespaceName;
                tempNS += o.prettyName(ns->name());
                tempNS += QLatin1String("::");
                if (const Class *cl = findClass(ns, className, &tempNS)) {
                    *namespaceName = tempNS;
                    return cl;
                }
            } // member is namespave
        } // member is no class
    } // for members
    return 0;
}

static const Function *findDeclaration(const Class *cl, const QString &functionName)
{
    const QString funName = QString::fromUtf8(QMetaObject::normalizedSignature(functionName.toUtf8()));
    const unsigned mCount = cl->memberCount();
    // we are interested only in declarations (can be decl of method or of a field)
    // we are only interested in declarations of methods
    const Overview overview;
    for (unsigned j = 0; j < mCount; j++) { // go through all members
        if (const Declaration *decl = cl->memberAt(j)->asDeclaration())
            if (const Function *fun = decl->type()->asFunction()) {
                // Format signature
                QString memberFunction = overview.prettyName(fun->name());
                memberFunction += QLatin1Char('(');
                const uint aCount = fun->argumentCount();
                for (uint i = 0; i < aCount; i++) { // we build argument types string
                    const Argument *arg = fun->argumentAt(i)->asArgument();
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
    return 0;
}

// TODO: remove me, see below
static bool isCompatible(const Name *name, const Name *otherName)
{
    if (const NameId *nameId = name->asNameId()) {
        if (const TemplateNameId *otherTemplId = otherName->asTemplateNameId())
            return nameId->identifier()->isEqualTo(otherTemplId->identifier());
    } else if (const TemplateNameId *templId = name->asTemplateNameId()) {
        if (const NameId *otherNameId = otherName->asNameId())
            return templId->identifier()->isEqualTo(otherNameId->identifier());
    }

    return name->isEqualTo(otherName);
}

// TODO: remove me, see below
static bool isCompatible(const Function *definition, const Symbol *declaration, const QualifiedNameId *declarationName)
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
static Document::Ptr findDefinition(const Function *functionDeclaration, int *line)
{
    CppTools::CppModelManagerInterface *cppModelManager = cppModelManagerInstance();
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
    const Snapshot documents = cppModelManager->snapshot();
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

static inline ITextEditable *editableAt(const QString &fileName, int line, int column)
{
    return qobject_cast<ITextEditable *>(TextEditor::BaseTextEditor::openEditorAt(fileName, line, column));
}

static void addDeclaration(const QString &docFileName, const Class *cl, const QString &functionName)
{
    QString declaration = QLatin1String("void ");
    declaration += functionName;
    declaration += QLatin1String(";\n");

    // functionName comes already with argument names (if designer managed to
    // do that).  First, let's try to find any method which is a private slot
    // (then we don't need to add "private slots:" statement)
    const unsigned mCount = cl->memberCount();
    for (unsigned j = 0; j < mCount; j++) { // go through all members
        if (const Declaration *decl = cl->memberAt(j)->asDeclaration())
            if (const Function *fun = decl->type()->asFunction())  {
                // we are only interested in declarations of methods.
                // fun->column() returns always 0, what can cause trouble in case in one
                // line if there is: "private slots: void foo();"
                if (fun->isSlot() && fun->isPrivate()) {
                    if (ITextEditable *editable = editableAt(docFileName, fun->line(), fun->column()))
                        editable->insert(declaration + QLatin1String("    "));
                    return;
                }
            }
    }

    // We didn't find any method under "private slots:", let's add "private slots:". Below code
    // adds "private slots:" by the end of the class definition.

    if (ITextEditable *editable = editableAt(docFileName, cl->line(), cl->column())) {
        int classEndPosition = findClassEndPosition(editable->contents(), editable->position());
        if (classEndPosition >= 0) {
            int line, column;
            editable->convertPosition(classEndPosition, &line, &column); // converts back position into a line and column
            editable->gotoLine(line, column);  // go to position (we should be just before closing } of the class)
            editable->insert(QLatin1String("\nprivate slots:\n    ") + declaration);
        }
    }
}

static Document::Ptr addDefinition(const CPlusPlus::Snapshot &docTable,
                                   const QString &headerFileName, const QString &className,
                                   const QString &functionName, int *line)
{
    QString definition = QLatin1String("\nvoid ");
    definition += className;
    definition += QLatin1String("::");
    definition += functionName;
    definition += QLatin1String("\n{\n");
    definition += QString(indentation, QLatin1Char(' '));
    definition += QLatin1String("\n}\n");

    // we find all documents which include headerFileName
    const QList<Document::Ptr> docList = findDocumentsIncluding(docTable, headerFileName, false);
    if (docList.isEmpty())
        return Document::Ptr();

    QFileInfo headerFI(headerFileName);
    const QString headerBaseName = headerFI.baseName();
    const QString headerAbsolutePath = headerFI.absolutePath();
    foreach (const Document::Ptr &doc, docList) {
        const QFileInfo sourceFI(doc->fileName());
        // we take only those documents which has the same filename and path (maybe we don't need to compare the path???)
        if (headerBaseName == sourceFI.baseName() && headerAbsolutePath == sourceFI.absolutePath()) {
            if (ITextEditable *editable = editableAt(doc->fileName(), 0, 0)) {
                const QString contents = editable->contents();
                int column;
                editable->convertPosition(contents.length(), line, &column);
                editable->gotoLine(*line, column);
                editable->insert(definition);
                *line += 1;
            }
            return doc;
        }
    }
    return Document::Ptr();
}

// Insert the parameter names into a signature, "void foo(bool)" ->
// "void foo(bool checked)"
static QString addParameterNames(const QString &functionSignature, const QStringList &parameterNames)
{
    const int firstParen = functionSignature.indexOf(QLatin1Char('('));
    QString functionName = functionSignature.left(firstParen + 1);
    QString argumentsString = functionSignature.mid(firstParen + 1);
    const int lastParen = argumentsString.lastIndexOf(QLatin1Char(')'));
    if (lastParen != -1)
        argumentsString.truncate(lastParen);
    const QStringList arguments = argumentsString.split(QLatin1Char(','), QString::SkipEmptyParts);
    const int pCount = parameterNames.count();
    const int aCount = arguments.count();
    for (int i = 0; i < aCount; ++i) {
        if (i > 0)
            functionName += QLatin1String(", ");
        functionName += arguments.at(i);
        if (i < pCount) {
            functionName += QLatin1Char(' ');
            functionName += parameterNames.at(i);
        }
    }
    functionName += QLatin1Char(')');
    return functionName;
}

// Recursively find a class definition in the document passed on or in its
// included files (going down [maxIncludeDepth] includes) and return a pair
// of <Class*, Document>.

typedef QPair<const Class *, Document::Ptr> ClassDocumentPtrPair;

static ClassDocumentPtrPair
        findClassRecursively(const CPlusPlus::Snapshot &docTable,
                             const Document::Ptr &doc, const QString &className,
                             unsigned maxIncludeDepth, QString *namespaceName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << doc->fileName() << maxIncludeDepth;
    // Check document
    if (const Class *cl = findClass(doc->globalNamespace(), className, namespaceName))
        return ClassDocumentPtrPair(cl, doc);
    if (maxIncludeDepth) {
        // Check the includes
        const unsigned recursionMaxIncludeDepth = maxIncludeDepth - 1u;
        foreach (const QString &include, doc->includedFiles()) {
            const CPlusPlus::Snapshot::const_iterator it = docTable.constFind(include);
            if (it != docTable.constEnd()) {
                const Document::Ptr includeDoc = it.value();
                const ClassDocumentPtrPair irc = findClassRecursively(docTable, it.value(), className, recursionMaxIncludeDepth, namespaceName);
                if (irc.first)
                    return irc;
            }
        }
    }
    return ClassDocumentPtrPair(0, Document::Ptr());
}

void WorkbenchIntegration::slotNavigateToSlot(const QString &objectName, const QString &signalSignature,
        const QStringList &parameterNames)
{
    QString errorMessage;
    if (!navigateToSlot(objectName, signalSignature, parameterNames, &errorMessage) && !errorMessage.isEmpty()) {
        QMessageBox::warning(m_few->designerEditor()->topLevel(), tr("Error finding/adding a slot."), errorMessage);
    }
}

// Build name of the class as generated by uic, insert Ui namespace
// "foo::bar::form" -> "foo::bar::Ui::form"

static inline QString uiClassName(QString formObjectName)
{
    const int indexOfScope = formObjectName.lastIndexOf(QLatin1String("::"));
    const int uiNameSpaceInsertionPos = indexOfScope >= 0 ? indexOfScope : 0;
    formObjectName.insert(uiNameSpaceInsertionPos, QLatin1String("Ui::"));
    return formObjectName;
}

bool WorkbenchIntegration::navigateToSlot(const QString &objectName,
                                          const QString &signalSignature,
                                          const QStringList &parameterNames,
                                          QString *errorMessage)
{
    const QString currentUiFile = m_few->activeFormWindow()->file()->fileName();

    // TODO: we should pass to findDocumentsIncluding an absolute path to generated .h file from ui.
    // Currently we are guessing the name of ui_<>.h file and pass the file name only to the findDocumentsIncluding().
    // The idea is that the .pro file knows if the .ui files is inside, and the .pro file knows it will
    // be generating the ui_<>.h file for it, and the .pro file knows what the generated file's name and its absolute path will be.
    // So we should somehow get that info from project manager (?)
    const QFileInfo fi(currentUiFile);
    const QString uicedName = QLatin1String("ui_") + fi.baseName() + QLatin1String(".h");

    // take all docs

    const CPlusPlus::Snapshot docTable = cppModelManagerInstance()->snapshot();
    QList<Document::Ptr> docList = findDocumentsIncluding(docTable, uicedName, true); // change to false when we know the absolute path to generated ui_<>.h file

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << objectName << signalSignature << "Looking for " << uicedName << " returned " << docList.size();
    if (docList.isEmpty()) {
        *errorMessage = tr("No documents matching %1 could be found.").arg(uicedName);
        return false;
    }

    QDesignerFormWindowInterface *fwi = m_few->activeFormWindow()->formWindow();

    const QString uiClass = uiClassName(fwi->mainContainer()->objectName());

    if (Designer::Constants::Internal::debug)
        qDebug() << "Checking docs for " << uiClass;

    // Find the class definition in the file itself or in the directly
    // included files (order 1).
    QString namespaceName;
    const Class *cl = 0;
    Document::Ptr doc;

    foreach (const Document::Ptr &d, docList) {
        const ClassDocumentPtrPair cd = findClassRecursively(docTable, d, uiClass, 1u , &namespaceName);
        if (cd.first) {
            cl = cd.first;
            doc = cd.second;
            break;
        }
    }
    if (!cl) {
        *errorMessage = msgClassNotFound(uiClass, docList);
        return false;
    }

    Overview o;
    const QString className = namespaceName + o.prettyName(cl->name());

    const QString functionName = QLatin1String("on_") + objectName + QLatin1Char('_') + signalSignature;
    const QString functionNameWithParameterNames = addParameterNames(functionName, parameterNames);

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << "Found " << uiClass << doc->fileName() << " checking " << functionName  << functionNameWithParameterNames;

    int line = 0;
    Document::Ptr sourceDoc;

    if (const Function *fun = findDeclaration(cl, functionName)) {
        sourceDoc = findDefinition(fun, &line);
        if (!sourceDoc) {
            // add function definition to cpp file
            sourceDoc = addDefinition(docTable, doc->fileName(), className, functionNameWithParameterNames, &line);
        }
    } else {
        // add function declaration to cl
        addDeclaration(doc->fileName(), cl, functionNameWithParameterNames);

        // add function definition to cpp file
        sourceDoc = addDefinition(docTable, doc->fileName(), className, functionNameWithParameterNames, &line);
    }

    if (!sourceDoc) {
        *errorMessage = tr("Unable to add the method definition.");
        return false;
    }

    // jump to function definition, position within code
    TextEditor::BaseTextEditor::openEditorAt(sourceDoc->fileName(), line + 2, indentation);

    return true;
}
