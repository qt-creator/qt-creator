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

#include "qtcreatorintegration.h"
#include "formwindoweditor.h"
#include "formeditorw.h"
#include "editordata.h"
#include <widgethost.h>
#include <designer/cpp/formclasswizardpage.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/cppworkingcopy.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/symbolfinder.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

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
using namespace ProjectExplorer;

static QString msgClassNotFound(const QString &uiClassName, const QList<Document::Ptr> &docList)
{
    QString files;
    for (const Document::Ptr &doc : docList) {
        files += '\n';
        files += QDir::toNativeSeparators(doc->fileName());
    }
    return QtCreatorIntegration::tr(
        "The class containing \"%1\" could not be found in %2.\n"
        "Please verify the #include-directives.")
        .arg(uiClassName, files);
}

QtCreatorIntegration::QtCreatorIntegration(QDesignerFormEditorInterface *core, QObject *parent)
    : QDesignerIntegration(core, parent)
{
    setResourceFileWatcherBehaviour(ReloadResourceFileSilently);
    Feature f = features();
    f |= SlotNavigationFeature;
    f &= ~ResourceEditorFeature;
    setFeatures(f);

    connect(this, QOverload<const QString &, const QString &, const QStringList &>::of
                       (&QDesignerIntegrationInterface::navigateToSlot),
            this, &QtCreatorIntegration::slotNavigateToSlot);
    connect(this, &QtCreatorIntegration::helpRequested,
            this, &QtCreatorIntegration::slotDesignerHelpRequested);
    slotSyncSettingsToDesigner();
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &QtCreatorIntegration::slotSyncSettingsToDesigner);
}

void QtCreatorIntegration::slotDesignerHelpRequested(const QString &manual, const QString &document)
{
    // Pass on as URL.
    emit creatorHelpRequested(QUrl(QString::fromLatin1("qthelp://com.trolltech.%1/qdoc/%2")
        .arg(manual, document)));
}

void QtCreatorIntegration::updateSelection()
{
    if (SharedTools::WidgetHost *host = FormEditorW::activeWidgetHost())
        host->updateFormWindowSelectionHandles(true);
    QDesignerIntegration::updateSelection();
}

QWidget *QtCreatorIntegration::containerWindow(QWidget * /*widget*/) const
{
    if (SharedTools::WidgetHost *host = FormEditorW::activeWidgetHost())
        return host->integrationContainer();
    return nullptr;
}

static QList<Document::Ptr> findDocumentsIncluding(const Snapshot &docTable,
                                                   const QString &fileName, bool checkFileNameOnly)
{
    QList<Document::Ptr> docList;
    for (const Document::Ptr &doc : docTable) { // we go through all documents
        const QList<Document::Include> includes = doc->resolvedIncludes()
            + doc->unresolvedIncludes();
        for (const Document::Include &include : includes) {
            if (checkFileNameOnly) {
                const QFileInfo fi(include.unresolvedFileName());
                if (fi.fileName() == fileName) { // we are only interested in docs which includes fileName only
                    docList.append(doc);
                }
            } else {
                if (include.resolvedFileName() == fileName)
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
    for (int b = 0; b < baseClassCount; ++b)
        if (o.prettyName(klass->baseClassAt(b)->name()) == baseClass)
            return true;
    return false;
}

QString fullyQualifiedName(const LookupContext &context, const Name *name, Scope *scope)
{
    if (!name || !scope)
        return QString();

    const QList<LookupItem> items = context.lookup(name, scope);
    if (items.isEmpty()) // "ui_xxx.h" might not be generated and nothing is forward declared.
        return Overview().prettyName(name);
    Symbol *symbol = items.first().declaration();
    return Overview().prettyName(LookupContext::fullyQualifiedName(symbol));
}

// Find class definition in namespace (that is, the outer class
// containing a member of the desired class type) or inheriting the desired class
// in case of forms using the Multiple Inheritance approach
static const Class *findClass(const Namespace *parentNameSpace, const LookupContext &context,
                              const QString &className)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << className;

    const Overview o;
    const int namespaceMemberCount = parentNameSpace->memberCount();
    for (int i = 0; i < namespaceMemberCount; ++i) { // we go through all namespace members
        const Symbol *sym = parentNameSpace->memberAt(i);
        // we have found a class - we are interested in classes only
        if (const Class *cl = sym->asClass()) {
            // 1) we go through class members
            const int classMemberCount = cl->memberCount();
            for (int j = 0; j < classMemberCount; ++j)
                if (Declaration *decl = cl->memberAt(j)->asDeclaration()) {
                // we want to know if the class contains a member (so we look into
                // a declaration) of uiClassName type
                    QString nameToMatch;
                    if (const NamedType *nt = decl->type()->asNamedType()) {
                        nameToMatch = fullyQualifiedName(context, nt->name(),
                                                         decl->enclosingScope());
                    // handle pointers to member variables
                    } else if (PointerType *pt = decl->type()->asPointerType()) {
                        if (NamedType *nt = pt->elementType()->asNamedType()) {
                            nameToMatch = fullyQualifiedName(context, nt->name(),
                                                             decl->enclosingScope());
                        }
                    }
                    if (!nameToMatch.isEmpty() && className == nameToMatch)
                        return cl;
                } // decl
            // 2) does it inherit the desired class
            if (inherits(o, cl, className))
                return cl;
        } else {
            // Check namespaces
            if (const Namespace *ns = sym->asNamespace()) {
                if (const Class *cl = findClass(ns, context, className))
                    return cl;
            } // member is namespace
        } // member is no class
    } // for members
    return nullptr;
}

static Function *findDeclaration(const Class *cl, const QString &functionName)
{
    const QString funName = QString::fromUtf8(QMetaObject::normalizedSignature(functionName.toUtf8()));
    const int mCount = cl->memberCount();
    // we are interested only in declarations (can be decl of function or of a field)
    // we are only interested in declarations of functions
    const Overview overview;
    for (int j = 0; j < mCount; ++j) { // go through all members
        if (Declaration *decl = cl->memberAt(j)->asDeclaration())
            if (Function *fun = decl->type()->asFunctionType()) {
                // Format signature
                QString memberFunction = overview.prettyName(fun->name());
                memberFunction += '(';
                const int aCount = fun->argumentCount();
                for (int i = 0; i < aCount; i++) { // we build argument types string
                    const Argument *arg = fun->argumentAt(i)->asArgument();
                    if (i > 0)
                        memberFunction += ',';
                    memberFunction += overview.prettyType(arg->type());
                }
                memberFunction += ')';
                // we compare normalized signatures
                memberFunction = QString::fromUtf8(QMetaObject::normalizedSignature(memberFunction.toUtf8()));
                if (memberFunction == funName) // we match function names and argument lists
                    return fun;
            }
    }
    return nullptr;
}

static inline BaseTextEditor *editorAt(const QString &fileName, int line, int column)
{
    return qobject_cast<BaseTextEditor *>(Core::EditorManager::openEditorAt(fileName, line, column,
                                                                         Utils::Id(),
                                                                         Core::EditorManager::DoNotMakeVisible));
}

static void addDeclaration(const Snapshot &snapshot,
                           const QString &fileName,
                           const Class *cl,
                           const QString &functionName)
{
    const QString declaration = "void " + functionName + ";\n";

    CppTools::CppRefactoringChanges refactoring(snapshot);
    CppTools::InsertionPointLocator find(refactoring);
    const CppTools::InsertionLocation loc = find.methodDeclarationInClass(
                fileName, cl, CppTools::InsertionPointLocator::PrivateSlot);

    //
    //! \todo change this to use the Refactoring changes.
    //

    if (BaseTextEditor *editor = editorAt(fileName, loc.line(), loc.column() - 1)) {
        QTextCursor tc = editor->textCursor();
        int pos = tc.position();
        tc.beginEditBlock();
        tc.insertText(loc.prefix() + declaration + loc.suffix());
        tc.setPosition(pos, QTextCursor::KeepAnchor);
        editor->textDocument()->autoIndent(tc);
        tc.endEditBlock();
    }
}

static QString addConstRefIfNeeded(const QString &argument)
{
    if (argument.startsWith("const ") || argument.endsWith('&') || argument.endsWith('*'))
        return argument;

    // for those types we don't want to add "const &"
    static const QStringList nonConstRefs = QStringList({"bool", "int", "uint", "float", "double",
                                                         "long", "short", "char", "signed",
                                                         "unsigned", "qint64", "quint64"});

    for (int i = 0; i < nonConstRefs.count(); i++) {
        const QString &nonConstRef = nonConstRefs.at(i);
        if (argument == nonConstRef || argument.startsWith(nonConstRef + ' '))
            return argument;
    }
    return "const " + argument + '&';
}

static QString formatArgument(const QString &argument)
{
    QString formattedArgument = argument;
    int i = argument.count();
    while (i > 0) { // from the end of the "argument" string
        i--;
        const QChar c = argument.at(i); // take the char
        if (c != '*' && c != '&') { // if it's not the * or &
            formattedArgument.insert(i + 1, ' '); // insert space after that char or just append space (to separate it from the parameter name)
            break;
        }
    }
    return formattedArgument;
}

// Insert the parameter names into a signature, "void foo(bool)" ->
// "void foo(bool checked)"
static QString addParameterNames(const QString &functionSignature, const QStringList &parameterNames)
{
    const int firstParen = functionSignature.indexOf('(');
    QString functionName = functionSignature.left(firstParen + 1);
    QString argumentsString = functionSignature.mid(firstParen + 1);
    const int lastParen = argumentsString.lastIndexOf(')');
    if (lastParen != -1)
        argumentsString.truncate(lastParen);
    const QStringList arguments = argumentsString.split(',', Qt::SkipEmptyParts);
    const int pCount = parameterNames.count();
    const int aCount = arguments.count();
    for (int i = 0; i < aCount; ++i) {
        if (i > 0)
            functionName += ", ";
        const QString argument = addConstRefIfNeeded(arguments.at(i));
        functionName += formatArgument(argument);
        if (i < pCount) {
            // prepare parameterName
            QString parameterName = parameterNames.at(i);
            if (parameterName.isEmpty()) {
                const QString generatedName = "arg" + QString::number(i + 1);
                if (!parameterNames.contains(generatedName))
                    parameterName = generatedName;
            }
            // add parameterName if not empty
            if (!parameterName.isEmpty())
                functionName += parameterName;
        }
    }
    functionName += ')';
    return functionName;
}

// Recursively find a class definition in the document passed on or in its
// included files (going down [maxIncludeDepth] includes) and return a pair
// of <Class*, Document>.

using ClassDocumentPtrPair = QPair<const Class *, Document::Ptr>;

static ClassDocumentPtrPair
        findClassRecursively(const LookupContext &context, const QString &className,
                             unsigned maxIncludeDepth)
{
    const Document::Ptr doc = context.thisDocument();
    const Snapshot docTable = context.snapshot();
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << doc->fileName() << className << maxIncludeDepth;
    // Check document
    if (const Class *cl = findClass(doc->globalNamespace(), context, className))
        return ClassDocumentPtrPair(cl, doc);
    if (maxIncludeDepth) {
        // Check the includes
        const unsigned recursionMaxIncludeDepth = maxIncludeDepth - 1u;
        const auto includedFiles = doc->includedFiles();
        for (const QString &include : includedFiles) {
            const Snapshot::const_iterator it = docTable.find(include);
            if (it != docTable.end()) {
                const Document::Ptr &includeDoc = it.value();
                LookupContext context(includeDoc, docTable);
                const ClassDocumentPtrPair irc = findClassRecursively(context, className,
                    recursionMaxIncludeDepth);
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
        QMessageBox::warning(FormEditorW::designerEditor()->topLevel(), tr("Error finding/adding a slot."), errorMessage);
}

// Build name of the class as generated by uic, insert Ui namespace
// "foo::bar::form" -> "foo::bar::Ui::form"

static inline QString uiClassName(QString formObjectName)
{
    const int indexOfScope = formObjectName.lastIndexOf("::");
    const int uiNameSpaceInsertionPos = indexOfScope >= 0 ? indexOfScope + 2 : 0;
    formObjectName.insert(uiNameSpaceInsertionPos, "Ui::");
    return formObjectName;
}

static Document::Ptr getParsedDocument(const QString &fileName,
                                       CppTools::WorkingCopy &workingCopy,
                                       Snapshot &snapshot)
{
    QByteArray src;
    if (workingCopy.contains(fileName)) {
        src = workingCopy.source(fileName);
    } else {
        Utils::FileReader reader;
        if (reader.fetch(fileName)) // ### FIXME error reporting
            src = QString::fromLocal8Bit(reader.data()).toUtf8();
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
    using DocumentMap = QMap<int, Document::Ptr>;

    const Utils::FilePath currentUiFile = FormEditorW::activeEditor()->document()->filePath();
#if 0
    return Designer::Internal::navigateToSlot(currentUiFile.toString(), objectName,
                                              signalSignature, parameterNames, errorMessage);
#endif
    // TODO: we should pass to findDocumentsIncluding an absolute path to generated .h file from ui.
    // Currently we are guessing the name of ui_<>.h file and pass the file name only to the findDocumentsIncluding().
    // The idea is that the .pro file knows if the .ui files is inside, and the .pro file knows it will
    // be generating the ui_<>.h file for it, and the .pro file knows what the generated file's name and its absolute path will be.
    // So we should somehow get that info from project manager (?)
    const QFileInfo fi = currentUiFile.toFileInfo();
    const QString uiFolder = fi.absolutePath();
    const QString uicedName = "ui_" + fi.completeBaseName() + ".h";

    // Retrieve code model snapshot restricted to project of ui file or the working copy.
    Snapshot docTable = CppTools::CppModelManager::instance()->snapshot();
    Snapshot newDocTable;
    const Project *uiProject = SessionManager::projectForFile(currentUiFile);
    if (uiProject) {
        for (Snapshot::const_iterator i = docTable.begin(), ei = docTable.end(); i != ei; ++i) {
            const Project *project = SessionManager::projectForFile(i.key());
            if (project == uiProject)
                newDocTable.insert(i.value());
        }
    } else {
        const Utils::FilePath configFileName =
                Utils::FilePath::fromString(CppTools::CppModelManager::configurationFileName());
        const CppTools::WorkingCopy::Table elements =
                CppTools::CppModelManager::instance()->workingCopy().elements();
        for (auto it = elements.cbegin(), end = elements.cend(); it != end; ++it) {
            const Utils::FilePath &fileName = it.key();
            if (fileName != configFileName)
                newDocTable.insert(docTable.document(fileName));
        }
    }
    docTable = newDocTable;

    // take all docs, find the ones that include the ui_xx.h.
    // Sort into a map, putting the ones whose path closely matches the ui-folder path
    // first in case there are project subdirectories that contain identical file names.
    const QList<Document::Ptr> docList = findDocumentsIncluding(docTable, uicedName, true); // change to false when we know the absolute path to generated ui_<>.h file
    DocumentMap docMap;
    for (const Document::Ptr &d : docList) {
        const QFileInfo docFi(d->fileName());
        docMap.insert(qAbs(docFi.absolutePath().compare(uiFolder, Qt::CaseInsensitive)), d);
    }

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << objectName << signalSignature << "Looking for " << uicedName << " returned " << docList.size();
    if (docMap.isEmpty()) {
        *errorMessage = tr("No documents matching \"%1\" could be found.\nRebuilding the project might help.").arg(uicedName);
        return false;
    }

    QDesignerFormWindowInterface *fwi = FormEditorW::activeWidgetHost()->formWindow();

    const QString uiClass = uiClassName(fwi->mainContainer()->objectName());

    if (Designer::Constants::Internal::debug)
        qDebug() << "Checking docs for " << uiClass;

    // Find the class definition (ui class defined as member or base class)
    // in the file itself or in the directly included files (order 1).
    const Class *cl = nullptr;
    Document::Ptr declDoc;
    for (const Document::Ptr &d : qAsConst(docMap)) {
        LookupContext context(d, docTable);
        const ClassDocumentPtrPair cd = findClassRecursively(context, uiClass, 1u);
        if (cd.first) {
            cl = cd.first;
            declDoc = cd.second;
            break;
        }
    }
    if (!cl) {
        *errorMessage = msgClassNotFound(uiClass, docList);
        return false;
    }

    const QString functionName = "on_" + objectName + '_' + signalSignature;
    const QString functionNameWithParameterNames = addParameterNames(functionName, parameterNames);

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << "Found " << uiClass << declDoc->fileName() << " checking " << functionName  << functionNameWithParameterNames;

    Function *fun = findDeclaration(cl, functionName);
    QString declFilePath;
    if (!fun) {
        // add function declaration to cl
        CppTools::WorkingCopy workingCopy = CppTools::CppModelManager::instance()->workingCopy();
        declFilePath = declDoc->fileName();
        getParsedDocument(declFilePath, workingCopy, docTable);
        addDeclaration(docTable, declFilePath, cl, functionNameWithParameterNames);

        // Re-load C++ documents.
        QList<Utils::FilePath> filePaths;
        for (auto it = docTable.begin(); it != docTable.end(); ++it)
            filePaths << it.key();
        workingCopy = CppTools::CppModelManager::instance()->workingCopy();
        docTable = CppTools::CppModelManager::instance()->snapshot();
        newDocTable = {};
        for (const auto &file : qAsConst(filePaths)) {
            const Document::Ptr doc = docTable.document(file);
            if (doc)
                newDocTable.insert(doc);
        }
        docTable = newDocTable;
        getParsedDocument(declFilePath, workingCopy, docTable);
        const Document::Ptr headerDoc = docTable.document(declFilePath);
        QTC_ASSERT(headerDoc, return false);
        LookupContext context(headerDoc, docTable);
        cl = findClass(headerDoc->globalNamespace(), context, uiClass);
        QTC_ASSERT(cl, return false);
        fun = findDeclaration(cl, functionName);
    } else {
        declFilePath = QLatin1String(fun->fileName());
    }
    QTC_ASSERT(fun, return false);

    CppTools::CppRefactoringChanges refactoring(docTable);
    CppTools::SymbolFinder symbolFinder;
    if (symbolFinder.findMatchingDefinition(fun, docTable, true))
        return true;
    const QString implFilePath = CppTools::correspondingHeaderOrSource(declFilePath);
    const CppTools::InsertionLocation location = CppTools::insertLocationForMethodDefinition
            (fun, false, CppTools::NamespaceHandling::CreateMissing, refactoring, implFilePath);

    if (BaseTextEditor *editor = editorAt(location.fileName(), location.line(), location.column())) {
        Overview o;
        const QString className = o.prettyName(cl->name());
        const QString definition = location.prefix() + "void " + className + "::"
            + functionNameWithParameterNames + "\n{\n" + QString(indentation, ' ') + "\n}\n"
            + location.suffix();
        editor->insert(definition);
        Core::EditorManager::openEditorAt(location.fileName(),
                                          location.line() + location.prefix().count('\n') + 2,
                                          indentation);
        return true;
    }

    *errorMessage = tr("Unable to add the method definition.");
    return false;
}

void QtCreatorIntegration::slotSyncSettingsToDesigner()
{
    // Set promotion-relevant parameters on integration.
    setHeaderSuffix(Utils::mimeTypeForName(CppTools::Constants::CPP_HEADER_MIMETYPE).preferredSuffix());
    setHeaderLowercase(FormClassWizardPage::lowercaseHeaderFiles());
}
