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

#include "formeditorplugin.h"
#include "formwindoweditor.h"
#include "formclasswizardpage.h"
#include "qtcreatorintegration.h"
#include "formeditorw.h"
#include "editordata.h"
#include "codemodelhelpers.h"
#include <widgethost.h>

#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/symbolfinder.h>
#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Overview.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/Name.h>
#include <cplusplus/Names.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Control.h>
#include <cplusplus/TranslationUnit.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetexteditor.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>
#include <utils/fileutils.h>

#include <QDesignerFormWindowInterface>
#include <QDesignerFormEditorInterface>

#include <QMessageBox>

#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QUrl>

enum { indentation = 4 };

using namespace Designer::Internal;
using namespace CPlusPlus;
using namespace TextEditor;

static QString msgClassNotFound(const QString &uiClassName, const QList<Document::Ptr> &docList)
{
    QString files;
    foreach (const Document::Ptr &doc, docList) {
        files += QLatin1Char('\n');
        files += QDir::toNativeSeparators(doc->fileName());
    }
    return QtCreatorIntegration::tr(
        "The class containing '%1' could not be found in %2.\n"
        "Please verify the #include-directives.")
        .arg(uiClassName, files);
}

QtCreatorIntegration::QtCreatorIntegration(QDesignerFormEditorInterface *core, FormEditorW *parent) :
#if QT_VERSION >= 0x050000
    QDesignerIntegration(core, parent),
#else
    qdesigner_internal::QDesignerIntegration(core, parent),
#endif
    m_few(parent)
{
#if QT_VERSION >= 0x050000
    setResourceFileWatcherBehaviour(ReloadResourceFileSilently);
    Feature f = features();
    f |= SlotNavigationFeature;
    f &= ~ResourceEditorFeature;
    setFeatures(f);
#else
    setResourceFileWatcherBehaviour(QDesignerIntegration::ReloadSilently);
    setResourceEditingEnabled(false);
    setSlotNavigationEnabled(true);
#endif
    connect(this, SIGNAL(navigateToSlot(QString,QString,QStringList)),
            this, SLOT(slotNavigateToSlot(QString,QString,QStringList)));
    connect(this, SIGNAL(helpRequested(QString,QString)),
            this, SLOT(slotDesignerHelpRequested(QString,QString)));
    slotSyncSettingsToDesigner();
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(slotSyncSettingsToDesigner()));
}

void QtCreatorIntegration::slotDesignerHelpRequested(const QString &manual, const QString &document)
{
    // Pass on as URL.
    emit creatorHelpRequested(QUrl(QString::fromLatin1("qthelp://com.trolltech.%1/qdoc/%2")
        .arg(manual, document)));
}

void QtCreatorIntegration::updateSelection()
{
    if (const EditorData ed = m_few->activeEditor())
        ed.widgetHost->updateFormWindowSelectionHandles(true);
#if QT_VERSION >= 0x050000
    QDesignerIntegration::updateSelection();
#else
    qdesigner_internal::QDesignerIntegration::updateSelection();
#endif
}

QWidget *QtCreatorIntegration::containerWindow(QWidget * /*widget*/) const
{
    if (const EditorData ed = m_few->activeEditor())
        return ed.widgetHost->integrationContainer();
    return 0;
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

// Does klass inherit baseClass?
static bool inherits(const Overview &o, const Class *klass, const QString &baseClass)
{
    const int baseClassCount = klass->baseClassCount();
    for (int b = 0; b < baseClassCount; b++)
        if (o.prettyName(klass->baseClassAt(b)->name()) == baseClass)
            return true;
    return false;
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

// Find class definition in namespace (that is, the outer class
// containing a member of the desired class type) or inheriting the desired class
// in case of forms using the Multiple Inheritance approach
static const Class *findClass(const Namespace *parentNameSpace, const QString &className, QString *namespaceName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << className;

    const Overview o;
    const unsigned namespaceMemberCount = parentNameSpace->memberCount();
    for (unsigned i = 0; i < namespaceMemberCount; ++i) { // we go through all namespace members
        const Symbol *sym = parentNameSpace->memberAt(i);
        // we have found a class - we are interested in classes only
        if (const Class *cl = sym->asClass()) {
            // 1) we go through class members
            const unsigned classMemberCount = cl->memberCount();
            for (unsigned j = 0; j < classMemberCount; ++j)
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
            // 2) does it inherit the desired class
            if (inherits(o, cl, className))
                return cl;
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

static Function *findDeclaration(const Class *cl, const QString &functionName)
{
    const QString funName = QString::fromUtf8(QMetaObject::normalizedSignature(functionName.toUtf8()));
    const unsigned mCount = cl->memberCount();
    // we are interested only in declarations (can be decl of method or of a field)
    // we are only interested in declarations of methods
    const Overview overview;
    for (unsigned j = 0; j < mCount; ++j) { // go through all members
        if (Declaration *decl = cl->memberAt(j)->asDeclaration())
            if (Function *fun = decl->type()->asFunctionType()) {
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

// TODO: remove me, this is taken from cppeditor.cpp. Find some common place for this method
static Document::Ptr findDefinition(Function *functionDeclaration, int *line)
{
    if (CppModelManagerInterface *cppModelManager = CppModelManagerInterface::instance()) {
        const Snapshot snapshot = cppModelManager->snapshot();
        CppTools::SymbolFinder symbolFinder;
        if (Symbol *def = symbolFinder.findMatchingDefinition(functionDeclaration, snapshot)) {
            if (line)
                *line = def->line();

            return snapshot.document(QString::fromUtf8(def->fileName(), def->fileNameLength()));
        }
    }

    return Document::Ptr();
}

static inline ITextEditor *editableAt(const QString &fileName, int line, int column)
{
    return qobject_cast<ITextEditor *>(TextEditor::BaseTextEditorWidget::openEditorAt(fileName, line, column));
}

static void addDeclaration(const Snapshot &snapshot,
                           const QString &fileName,
                           const Class *cl,
                           const QString &functionName)
{
    QString declaration = QLatin1String("void ");
    declaration += functionName;
    declaration += QLatin1String(";\n");

    CppTools::CppRefactoringChanges refactoring(snapshot);
    CppTools::InsertionPointLocator find(refactoring);
    const CppTools::InsertionLocation loc = find.methodDeclarationInClass(
                fileName, cl, CppTools::InsertionPointLocator::PrivateSlot);

    //
    //! \todo change this to use the Refactoring changes.
    //

    if (ITextEditor *editable = editableAt(fileName, loc.line(), loc.column() - 1)) {
        BaseTextEditorWidget *editor = qobject_cast<BaseTextEditorWidget *>(editable->widget());
        if (editor) {
            QTextCursor tc = editor->textCursor();
            int pos = tc.position();
            tc.beginEditBlock();
            tc.insertText(loc.prefix() + declaration + loc.suffix());
            tc.setPosition(pos, QTextCursor::KeepAnchor);
            editor->indentInsertedText(tc);
            tc.endEditBlock();
        }
    }
}

static Document::Ptr addDefinition(const CPlusPlus::Snapshot &docTable,
                                   const QString &headerFileName,
                                   const QString &className,
                                   const QString &functionName,
                                   int *line)
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
    const QString headerBaseName = headerFI.completeBaseName();
    foreach (const Document::Ptr &doc, docList) {
        const QFileInfo sourceFI(doc->fileName());
        // we take only those documents which have the same filename
        if (headerBaseName == sourceFI.baseName()) {
            //
            //! \todo change this to use the Refactoring changes.
            //

            if (ITextEditor *editable = editableAt(doc->fileName(), 0, 0)) {

                //
                //! \todo use the InsertionPointLocator to insert at the correct place.
                // (we'll have to extend that class first to do definition insertions)

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

static QString addConstRefIfNeeded(const QString &argument)
{
    if (argument.startsWith(QLatin1String("const "))
            || argument.endsWith(QLatin1Char('&'))
            || argument.endsWith(QLatin1Char('*')))
        return argument;

    // for those types we don't want to add "const &"
    static const QStringList nonConstRefs = QStringList()
            << QLatin1String("bool")
            << QLatin1String("int")
            << QLatin1String("uint")
            << QLatin1String("float")
            << QLatin1String("double")
            << QLatin1String("long")
            << QLatin1String("short")
            << QLatin1String("char")
            << QLatin1String("signed")
            << QLatin1String("unsigned")
            << QLatin1String("qint64")
            << QLatin1String("quint64");

    for (int i = 0; i < nonConstRefs.count(); i++) {
        const QString nonConstRef = nonConstRefs.at(i);
        if (argument == nonConstRef || argument.startsWith(nonConstRef + QLatin1Char(' ')))
            return argument;
    }
    return QLatin1String("const ") + argument + QLatin1Char('&');
}

static QString formatArgument(const QString &argument)
{
    QString formattedArgument = argument;
    int i = argument.count();
    while (i > 0) { // from the end of the "argument" string
        i--;
        const QChar c = argument.at(i); // take the char
        if (c != QLatin1Char('*') && c != QLatin1Char('&')) { // if it's not the * or &
            formattedArgument.insert(i + 1, QLatin1Char(' ')); // insert space after that char or just append space (to separate it from the parameter name)
            break;
        }
    }
    return formattedArgument;
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
        const QString argument = addConstRefIfNeeded(arguments.at(i));
        functionName += formatArgument(argument);
        if (i < pCount) {
            // prepare parameterName
            QString parameterName = parameterNames.at(i);
            if (parameterName.isEmpty()) {
                const QString generatedName = QLatin1String("arg") + QString::number(i + 1);
                if (!parameterNames.contains(generatedName))
                    parameterName = generatedName;
            }
            // add parameterName if not empty
            if (!parameterName.isEmpty())
                functionName += parameterName;
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
        qDebug() << Q_FUNC_INFO << doc->fileName() << className << maxIncludeDepth;
    // Check document
    if (const Class *cl = findClass(doc->globalNamespace(), className, namespaceName))
        return ClassDocumentPtrPair(cl, doc);
    if (maxIncludeDepth) {
        // Check the includes
        const unsigned recursionMaxIncludeDepth = maxIncludeDepth - 1u;
        foreach (const QString &include, doc->includedFiles()) {
            const CPlusPlus::Snapshot::const_iterator it = docTable.find(include);
            if (it != docTable.end()) {
                const Document::Ptr includeDoc = it.value();
                const ClassDocumentPtrPair irc = findClassRecursively(docTable, it.value(), className, recursionMaxIncludeDepth, namespaceName);
                if (irc.first)
                    return irc;
            }
        }
    }
    return ClassDocumentPtrPair(0, Document::Ptr());
}

void QtCreatorIntegration::slotNavigateToSlot(const QString &objectName, const QString &signalSignature,
        const QStringList &parameterNames)
{
    QString errorMessage;
    if (!navigateToSlot(objectName, signalSignature, parameterNames, &errorMessage) && !errorMessage.isEmpty())
        QMessageBox::warning(m_few->designerEditor()->topLevel(), tr("Error finding/adding a slot."), errorMessage);
}

// Build name of the class as generated by uic, insert Ui namespace
// "foo::bar::form" -> "foo::bar::Ui::form"

static inline QString uiClassName(QString formObjectName)
{
    const int indexOfScope = formObjectName.lastIndexOf(QLatin1String("::"));
    const int uiNameSpaceInsertionPos = indexOfScope >= 0 ? indexOfScope + 2 : 0;
    formObjectName.insert(uiNameSpaceInsertionPos, QLatin1String("Ui::"));
    return formObjectName;
}

static Document::Ptr getParsedDocument(const QString &fileName, CppModelManagerInterface::WorkingCopy &workingCopy, Snapshot &snapshot)
{
    QString src;
    if (workingCopy.contains(fileName)) {
        src = workingCopy.source(fileName);
    } else {
        Utils::FileReader reader;
        if (reader.fetch(fileName)) // ### FIXME error reporting
            src = QString::fromLocal8Bit(reader.data()); // ### FIXME encoding
    }

    Document::Ptr doc = snapshot.preprocessedDocument(src, fileName);
    doc->check();
    snapshot.insert(doc);
    return doc;
}

// Goto slot invoked by the designer context menu. Either navigates
// to an existing slot function or create a new one.

bool QtCreatorIntegration::navigateToSlot(const QString &objectName,
                                          const QString &signalSignature,
                                          const QStringList &parameterNames,
                                          QString *errorMessage)
{
    typedef QMap<int, Document::Ptr> DocumentMap;

    const EditorData ed = m_few->activeEditor();
    QTC_ASSERT(ed, return false);
    const QString currentUiFile = ed.formWindowEditor->document()->fileName();
#if 0
    return Designer::Internal::navigateToSlot(currentUiFile, objectName, signalSignature, parameterNames, errorMessage);
#endif
    // TODO: we should pass to findDocumentsIncluding an absolute path to generated .h file from ui.
    // Currently we are guessing the name of ui_<>.h file and pass the file name only to the findDocumentsIncluding().
    // The idea is that the .pro file knows if the .ui files is inside, and the .pro file knows it will
    // be generating the ui_<>.h file for it, and the .pro file knows what the generated file's name and its absolute path will be.
    // So we should somehow get that info from project manager (?)
    const QFileInfo fi(currentUiFile);
    const QString uiFolder = fi.absolutePath();
    const QString uicedName = QLatin1String("ui_") + fi.completeBaseName() + QLatin1String(".h");

    // Retrieve code model snapshot restricted to project of ui file.
    const ProjectExplorer::Project *uiProject = ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projectForFile(currentUiFile);
    if (!uiProject) {
        *errorMessage = tr("Internal error: No project could be found for %1.").arg(currentUiFile);
        return false;
    }
    CPlusPlus::Snapshot docTable = CppModelManagerInterface::instance()->snapshot();
    CPlusPlus::Snapshot newDocTable;

    for  (CPlusPlus::Snapshot::iterator it = docTable.begin(); it != docTable.end(); ++it) {
        const ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::instance()->session()->projectForFile(it.key());
        if (project == uiProject)
            newDocTable.insert(it.value());
    }

    docTable = newDocTable;

    // take all docs, find the ones that include the ui_xx.h.
    // Sort into a map, putting the ones whose path closely matches the ui-folder path
    // first in case there are project subdirectories that contain identical file names.
    const QList<Document::Ptr> docList = findDocumentsIncluding(docTable, uicedName, true); // change to false when we know the absolute path to generated ui_<>.h file
    DocumentMap docMap;
    foreach (const Document::Ptr &d, docList) {
        const QFileInfo docFi(d->fileName());
        docMap.insert(qAbs(docFi.absolutePath().compare(uiFolder, Qt::CaseInsensitive)), d);
    }

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << objectName << signalSignature << "Looking for " << uicedName << " returned " << docList.size();
    if (docMap.isEmpty()) {
        *errorMessage = tr("No documents matching '%1' could be found.\nRebuilding the project might help.").arg(uicedName);
        return false;
    }

    QDesignerFormWindowInterface *fwi = ed.widgetHost->formWindow();

    const QString uiClass = uiClassName(fwi->mainContainer()->objectName());

    if (Designer::Constants::Internal::debug)
        qDebug() << "Checking docs for " << uiClass;

    // Find the class definition (ui class defined as member or base class)
    // in the file itself or in the directly included files (order 1).
    QString namespaceName;
    const Class *cl = 0;
    Document::Ptr doc;

    foreach (const Document::Ptr &d, docMap) {
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
    if (Designer::Constants::Internal::debug)
        qDebug() << "Found class  " << className << doc->fileName();

    const QString functionName = QLatin1String("on_") + objectName + QLatin1Char('_') + signalSignature;
    const QString functionNameWithParameterNames = addParameterNames(functionName, parameterNames);

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << "Found " << uiClass << doc->fileName() << " checking " << functionName  << functionNameWithParameterNames;

    int line = 0;
    Document::Ptr sourceDoc;

    if (Function *fun = findDeclaration(cl, functionName)) {
        sourceDoc = findDefinition(fun, &line);
        if (!sourceDoc) {
            // add function definition to cpp file
            sourceDoc = addDefinition(docTable, doc->fileName(), className, functionNameWithParameterNames, &line);
        }
    } else {
        // add function declaration to cl
        CppModelManagerInterface::WorkingCopy workingCopy =
            CppModelManagerInterface::instance()->workingCopy();
        const QString fileName = doc->fileName();
        getParsedDocument(fileName, workingCopy, docTable);
        addDeclaration(docTable, fileName, cl, functionNameWithParameterNames);

        // add function definition to cpp file
        sourceDoc = addDefinition(docTable, fileName, className, functionNameWithParameterNames, &line);
    }

    if (!sourceDoc) {
        *errorMessage = tr("Unable to add the method definition.");
        return false;
    }

    // jump to function definition, position within code
    TextEditor::BaseTextEditorWidget::openEditorAt(sourceDoc->fileName(), line + 2, indentation);

    return true;
}

void QtCreatorIntegration::slotSyncSettingsToDesigner()
{
#if QT_VERSION > 0x040800
    // Set promotion-relevant parameters on integration.
    const Core::MimeDatabase *mdb = Core::ICore::mimeDatabase();
    setHeaderSuffix(mdb->preferredSuffixByType(QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)));
    setHeaderLowercase(FormClassWizardPage::lowercaseHeaderFiles());
#endif
}
