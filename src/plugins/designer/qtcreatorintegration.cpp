// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtcreatorintegration.h"

#include "designerconstants.h"
#include "designertr.h"
#include "formeditor.h"
#include "formwindoweditor.h"

#include <widgethost.h>
#include <designer/cpp/formclasswizardpage.h>

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppeditorplugin.h>
#include <cppeditor/cppeditorwidget.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cppsemanticinfo.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/cppworkingcopy.h>
#include <cppeditor/insertionpointlocator.h>
#include <cppeditor/symbolfinder.h>

#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/extracompiler.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/temporaryfile.h>

#include <QDesignerFormWindowInterface>
#include <QDesignerFormEditorInterface>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QHash>
#include <QUrl>

#include <memory>

enum { indentation = 4 };

Q_LOGGING_CATEGORY(log, "qtc.designer", QtWarningMsg);

using namespace Designer::Internal;
using namespace CPlusPlus;
using namespace TextEditor;
using namespace ProjectExplorer;
using namespace Utils;

static QString msgClassNotFound(const QString &uiClassName, const QList<Document::Ptr> &docList)
{
    QString files;
    for (const Document::Ptr &doc : docList) {
        files += '\n';
        files += doc->filePath().toUserOutput();
    }
    return Designer::Tr::tr(
        "The class containing \"%1\" could not be found in %2.\n"
        "Please verify the #include-directives.")
        .arg(uiClassName, files);
}

static void reportRenamingError(const QString &oldName, const QString &reason)
{
    Core::MessageManager::writeFlashing(
                Designer::Tr::tr("Cannot rename UI symbol \"%1\" in C++ files: %2")
                .arg(oldName, reason));
}

class QtCreatorIntegration::Private
{
public:
    // See QTCREATORBUG-19141 for why this is necessary.
    QHash<QDesignerFormWindowInterface *, QPointer<ExtraCompiler>> extraCompilers;
    std::optional<bool> showPropertyEditorRenameWarning = false;
};

QtCreatorIntegration::QtCreatorIntegration(QDesignerFormEditorInterface *core, QObject *parent)
    : QDesignerIntegration(core, parent), d(new Private)
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

    // The problem is as follows:
    //   - If the user edits the object name in the property editor, the objectNameChanged() signal
    //     is emitted for every keystroke (QTCREATORBUG-19141). We should not try to rename
    //     in that case, because the signals will likely come in faster than the renaming
    //     procedure takes, putting the code model in some non-deterministic state.
    //   - Unfortunately, this condition is not trivial to detect, because the propertyChanged()
    //     signal is (somewhat surprisingly) emitted *after* objectNameChanged().
    //   - We can also not simply use a queued connection for objectNameChanged(), because then
    //     the ExtraCompiler might have run before our handler, and we won't find the old
    //     object name in the source code anymore.
    //  The solution is as follows:
    //   - Upon receiving objectNameChanged(), we retrieve the corresponding ExtraCompiler,
    //     block it and store it away. Then we invoke the actual handler delayed.
    //   - Upon receiving propertyChanged(), we check whether it refers to an object name change.
    //     If it does, we unblock the ExtraCompiler and remove it from our map.
    //   - When the real handler runs, it first checks for the ExtraCompiler. If it is not found,
    //     we don't do anything. Otherwise the actual renaming procedure is run.
    connect(this, &QtCreatorIntegration::objectNameChanged,
            this, &QtCreatorIntegration::handleSymbolRenameStage1);
    connect(this, &QtCreatorIntegration::propertyChanged,
            this, [this](QDesignerFormWindowInterface *formWindow, const QString &name,
                         const QVariant &) {
        qCDebug(log) << "got propertyChanged() signal" << name;
        if (name.endsWith("Name")) {
            if (const auto extraCompiler = d->extraCompilers.find(formWindow);
                    extraCompiler != d->extraCompilers.end()) {
                (*extraCompiler)->unblock();
                d->extraCompilers.erase(extraCompiler);
                if (d->showPropertyEditorRenameWarning)
                    d->showPropertyEditorRenameWarning = true;
            }
        }
    });
}

QtCreatorIntegration::~QtCreatorIntegration()
{
    delete d;
}

void QtCreatorIntegration::slotDesignerHelpRequested(const QString &manual, const QString &document)
{
    // Pass on as URL.
    emit creatorHelpRequested(QUrl(QString::fromLatin1("qthelp://com.trolltech.%1/qdoc/%2")
        .arg(manual, document)));
}

void QtCreatorIntegration::updateSelection()
{
    if (SharedTools::WidgetHost *host = activeWidgetHost())
        host->updateFormWindowSelectionHandles(true);
    QDesignerIntegration::updateSelection();
}

QWidget *QtCreatorIntegration::containerWindow(QWidget * /*widget*/) const
{
    if (SharedTools::WidgetHost *host = activeWidgetHost())
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
                if (include.resolvedFileName().path() == fileName)
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

static BaseTextEditor *editorAt(const FilePath &filePath, int line, int column)
{
    return qobject_cast<BaseTextEditor *>(
        Core::EditorManager::openEditorAt({filePath, line, column},
                                          Utils::Id(),
                                          Core::EditorManager::DoNotMakeVisible));
}

static void addDeclaration(const Snapshot &snapshot,
                           const FilePath &filePath,
                           const Class *cl,
                           const QString &functionName)
{
    const QString declaration = "void " + functionName + ";\n";

    CppEditor::CppRefactoringChanges refactoring(snapshot);
    CppEditor::InsertionPointLocator find(refactoring);
    const CppEditor::InsertionLocation loc = find.methodDeclarationInClass(
                filePath, cl, CppEditor::InsertionPointLocator::PrivateSlot);

    //
    //! \todo change this to use the Refactoring changes.
    //

    if (BaseTextEditor *editor = editorAt(filePath, loc.line(), loc.column() - 1)) {
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

    for (int i = 0; i < nonConstRefs.size(); i++) {
        const QString &nonConstRef = nonConstRefs.at(i);
        if (argument == nonConstRef || argument.startsWith(nonConstRef + ' '))
            return argument;
    }
    return "const " + argument + '&';
}

static QString formatArgument(const QString &argument)
{
    QString formattedArgument = argument;
    int i = argument.size();
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
    const int pCount = parameterNames.size();
    const int aCount = arguments.size();
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
        qDebug() << Q_FUNC_INFO << doc->filePath() << className << maxIncludeDepth;
    // Check document
    if (const Class *cl = findClass(doc->globalNamespace(), context, className))
        return ClassDocumentPtrPair(cl, doc);
    if (maxIncludeDepth) {
        // Check the includes
        const unsigned recursionMaxIncludeDepth = maxIncludeDepth - 1u;
        const FilePaths includedFiles = doc->includedFiles();
        for (const FilePath &include : includedFiles) {
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
        QMessageBox::warning(designerEditor()->topLevel(), Tr::tr("Error finding/adding a slot."), errorMessage);
}

// Build name of the class as generated by uic, insert Ui namespace
// "foo::bar::form" -> "foo::bar::Ui::form"

static inline const QStringList uiClassNames(QString formObjectName)
{
    const int indexOfScope = formObjectName.lastIndexOf("::");
    const int uiNameSpaceInsertionPos = indexOfScope >= 0 ? indexOfScope + 2 : 0;
    QString alt = formObjectName;
    formObjectName.insert(uiNameSpaceInsertionPos, "Ui::");
    alt.insert(uiNameSpaceInsertionPos, "Ui_");
    return {formObjectName, alt};
}

static Document::Ptr getParsedDocument(const FilePath &filePath,
                                       CppEditor::WorkingCopy &workingCopy,
                                       Snapshot &snapshot)
{
    QByteArray src;
    if (const auto source = workingCopy.source(filePath)) {
        src = *source;
    } else {
        Utils::FileReader reader;
        if (reader.fetch(filePath)) // ### FIXME error reporting
            src = QString::fromLocal8Bit(reader.data()).toUtf8();
    }

    Document::Ptr doc = snapshot.preprocessedDocument(src, filePath);
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

    const Utils::FilePath currentUiFile = activeEditor()->document()->filePath();
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
    Snapshot docTable = CppEditor::CppModelManager::snapshot();
    Snapshot newDocTable;
    const Project *uiProject = ProjectManager::projectForFile(currentUiFile);
    if (uiProject) {
        for (Snapshot::const_iterator i = docTable.begin(), ei = docTable.end(); i != ei; ++i) {
            const Project *project = ProjectManager::projectForFile(i.key());
            if (project == uiProject)
                newDocTable.insert(i.value());
        }
    } else {
        const FilePath configFileName = CppEditor::CppModelManager::configurationFileName();
        const CppEditor::WorkingCopy::Table elements =
                CppEditor::CppModelManager::workingCopy().elements();
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
        docMap.insert(qAbs(d->filePath().absolutePath().toString()
                           .compare(uiFolder, Qt::CaseInsensitive)), d);
    }

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << objectName << signalSignature << "Looking for " << uicedName << " returned " << docList.size();
    if (docMap.isEmpty()) {
        *errorMessage = Tr::tr("No documents matching \"%1\" could be found.\nRebuilding the project might help.").arg(uicedName);
        return false;
    }

    QDesignerFormWindowInterface *fwi = activeWidgetHost()->formWindow();

    QString uiClass;
    const Class *cl = nullptr;
    Document::Ptr declDoc;
    for (const QString &candidate : uiClassNames(fwi->mainContainer()->objectName())) {
        if (Designer::Constants::Internal::debug)
            qDebug() << "Checking docs for " << candidate;

        // Find the class definition (ui class defined as member or base class)
        // in the file itself or in the directly included files (order 1).
        for (const Document::Ptr &d : std::as_const(docMap)) {
            LookupContext context(d, docTable);
            const ClassDocumentPtrPair cd = findClassRecursively(context, candidate, 1u);
            if (cd.first) {
                cl = cd.first;
                declDoc = cd.second;
                break;
            }
        }
        if (cl) {
            uiClass = candidate;
            break;
        }

        if (errorMessage->isEmpty())
            *errorMessage = msgClassNotFound(candidate, docList);
    }
    if (!cl)
        return false;

    const QString functionName = "on_" + objectName + '_' + signalSignature;
    const QString functionNameWithParameterNames = addParameterNames(functionName, parameterNames);

    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << "Found " << uiClass << declDoc->filePath() << " checking " << functionName  << functionNameWithParameterNames;

    Function *fun = findDeclaration(cl, functionName);
    FilePath declFilePath;
    if (!fun) {
        // add function declaration to cl
        CppEditor::WorkingCopy workingCopy = CppEditor::CppModelManager::workingCopy();
        declFilePath = declDoc->filePath();
        getParsedDocument(declFilePath, workingCopy, docTable);
        addDeclaration(docTable, declFilePath, cl, functionNameWithParameterNames);

        // Re-load C++ documents.
        QList<Utils::FilePath> filePaths;
        for (auto it = docTable.begin(); it != docTable.end(); ++it)
            filePaths << it.key();
        workingCopy = CppEditor::CppModelManager::workingCopy();
        docTable = CppEditor::CppModelManager::snapshot();
        newDocTable = {};
        for (const auto &file : std::as_const(filePaths)) {
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
        declFilePath = FilePath::fromString(QLatin1String(fun->fileName()));
    }
    QTC_ASSERT(fun, return false);

    CppEditor::CppRefactoringChanges refactoring(docTable);
    CppEditor::SymbolFinder symbolFinder;
    if (const Function *funImpl = symbolFinder.findMatchingDefinition(fun, docTable, true)) {
        Core::EditorManager::openEditorAt(
            {FilePath::fromString(QString::fromUtf8(funImpl->fileName())), funImpl->line() + 2});
        return true;
    }
    const FilePath implFilePath = CppEditor::correspondingHeaderOrSource(declFilePath);
    const CppEditor::InsertionLocation location = CppEditor::insertLocationForMethodDefinition
            (fun, false, CppEditor::NamespaceHandling::CreateMissing, refactoring, implFilePath);

    if (BaseTextEditor *editor = editorAt(location.filePath(),
                                          location.line(), location.column())) {
        Overview o;
        const QString className = o.prettyName(cl->name());
        const QString definition = location.prefix() + "void " + className + "::"
            + functionNameWithParameterNames + "\n{\n" + QString(indentation, ' ') + "\n}\n"
            + location.suffix();
        editor->insert(definition);
        Core::EditorManager::openEditorAt({location.filePath(),
                                           int(location.line() + location.prefix().count('\n') + 2),
                                           indentation});
        return true;
    }

    *errorMessage = Tr::tr("Unable to add the method definition.");
    return false;
}

void QtCreatorIntegration::handleSymbolRenameStage1(
        QDesignerFormWindowInterface *formWindow, QObject *object,
        const QString &newName, const QString &oldName)
{
    const FilePath uiFile = FilePath::fromString(formWindow->fileName());
    qCDebug(log) << Q_FUNC_INFO << uiFile << object << oldName << newName;
    if (newName.isEmpty() || newName == oldName)
        return;

    // Get ExtraCompiler.
    const Project * const project = ProjectManager::projectForFile(uiFile);
    if (!project) {
        return reportRenamingError(oldName, Designer::Tr::tr("File \"%1\" not found in project.")
                                   .arg(uiFile.toUserOutput()));
    }
    const Target * const target = project->activeTarget();
    if (!target)
        return reportRenamingError(oldName, Designer::Tr::tr("No active target."));
    BuildSystem * const buildSystem = target->buildSystem();
    if (!buildSystem)
        return reportRenamingError(oldName, Designer::Tr::tr("No active build system."));
    ExtraCompiler * const ec = buildSystem->extraCompilerForSource(uiFile);
    if (!ec)
        return reportRenamingError(oldName, Designer::Tr::tr("Failed to find the ui header."));
    ec->block();
    d->extraCompilers.insert(formWindow, ec);
    qCDebug(log) << "\tfound extra compiler, scheduling stage 2";
    QMetaObject::invokeMethod(this, [this, formWindow, newName, oldName] {
        handleSymbolRenameStage2(formWindow, newName, oldName);
    }, Qt::QueuedConnection);
}

void QtCreatorIntegration::handleSymbolRenameStage2(
        QDesignerFormWindowInterface *formWindow, const QString &newName, const QString &oldName)
{
    // Retrieve and check previously stored ExtraCompiler.
    ExtraCompiler * const ec = d->extraCompilers.take(formWindow);
    if (!ec) {
        qCDebug(log) << "\tchange came from property editor, ignoring";
        if (d->showPropertyEditorRenameWarning && *d->showPropertyEditorRenameWarning) {
            d->showPropertyEditorRenameWarning.reset();
            reportRenamingError(oldName, Designer::Tr::tr("Renaming via the property editor "
                "cannot be synced with C++ code; see QTCREATORBUG-19141."
                " This message will not be repeated."));
        }
        return;
    }

    class ResourceHandler {
    public:
        ResourceHandler(ExtraCompiler *ec) : m_ec(ec) {}
        void setEditor(BaseTextEditor *editorToClose) { m_editorToClose = editorToClose; }
        void setTempFile(std::unique_ptr<TemporaryFile> &&tempFile) {
            m_tempFile = std::move(tempFile);
        }
        ~ResourceHandler()
        {
            if (m_ec)
                m_ec->unblock();
            if (m_editorToClose)
                Core::EditorManager::closeEditors({m_editorToClose}, false);
        }
    private:
        const QPointer<ExtraCompiler> m_ec;
        QPointer<BaseTextEditor> m_editorToClose;
        std::unique_ptr<TemporaryFile> m_tempFile;
    };
    const auto resourceHandler = std::make_shared<ResourceHandler>(ec);

    QTC_ASSERT(ec->targets().size() == 1, return);
    const FilePath uiHeader = ec->targets().first();
    qCDebug(log) << '\t' << uiHeader;
    const QByteArray virtualContent = ec->content(uiHeader);
    if (virtualContent.isEmpty()) {
        qCDebug(log) << "\textra compiler unexpectedly has no contents";
        return reportRenamingError(oldName,
                                   Designer::Tr::tr("Failed to retrieve ui header contents."));
    }

    // Secretly open ui header file contents in editor.
    // Use a temp file rather than the actual ui header path.
    const auto openFlags = Core::EditorManager::DoNotMakeVisible
            | Core::EditorManager::DoNotChangeCurrentEditor;
    std::unique_ptr<TemporaryFile> tempFile
            = std::make_unique<TemporaryFile>("XXXXXX" + uiHeader.fileName());
    QTC_ASSERT(tempFile->open(), return);
    qCDebug(log) << '\t' << tempFile->fileName();
    const auto editor = qobject_cast<BaseTextEditor *>(
                Core::EditorManager::openEditor(FilePath::fromString(tempFile->fileName()), {},
                                                openFlags));
    QTC_ASSERT(editor, return);
    resourceHandler->setTempFile(std::move(tempFile));
    resourceHandler->setEditor(editor);

    const auto editorWidget = qobject_cast<CppEditor::CppEditorWidget *>(editor->editorWidget());
    QTC_ASSERT(editorWidget && editorWidget->textDocument(), return);

    // Parse temp file with built-in code model. Pretend it's the real ui header.
    // In the case of clangd, this entails doing a "virtual rename" on the TextDocument,
    // as the LanguageClient cannot be forced into taking a document and assuming a different
    // file path.
    const bool usesClangd = CppEditor::CppModelManager::usesClangd(editorWidget->textDocument());
    if (usesClangd)
        editorWidget->textDocument()->setFilePath(uiHeader);
    editorWidget->textDocument()->setPlainText(QString::fromUtf8(virtualContent));
    Snapshot snapshot = CppEditor::CppModelManager::snapshot();
    snapshot.remove(uiHeader);
    snapshot.remove(editor->textDocument()->filePath());
    const Document::Ptr cppDoc = snapshot.preprocessedDocument(virtualContent, uiHeader);
    cppDoc->check();
    QTC_ASSERT(cppDoc && cppDoc->isParsed(), return);

    // Locate old identifier in ui header.
    const QByteArray oldNameBa = oldName.toUtf8();
    const Identifier oldIdentifier(oldNameBa.constData(), oldNameBa.size());
    QList<const Scope *> scopes{cppDoc->globalNamespace()};
    while (!scopes.isEmpty()) {
        const Scope * const scope = scopes.takeFirst();
        qCDebug(log) << '\t' << scope->memberCount();
        for (int i = 0; i < scope->memberCount(); ++i) {
            Symbol * const symbol = scope->memberAt(i);
            if (const Scope * const s = symbol->asScope())
                scopes << s;
            if (symbol->asNamespace())
                continue;
            qCDebug(log) << '\t' << Overview().prettyName(symbol->name());
            if (!symbol->name()->match(&oldIdentifier))
                continue;
            QTextCursor cursor(editorWidget->textCursor());
            cursor.setPosition(cppDoc->translationUnit()->getTokenPositionInDocument(
                                   symbol->sourceLocation(), editorWidget->document()));
            qCDebug(log) << '\t' << cursor.position() << cursor.blockNumber()
                         << cursor.positionInBlock();

            // Trigger non-interactive renaming. The callback is destructed after invocation,
            // closing the editor, removing the temp file and unblocking the extra compiler.
            // For the built-in code model, we must access the model manager directly,
            // as otherwise our file path trickery would be found out.
            const auto callback = [resourceHandler] { };
            if (usesClangd) {
                qCDebug(log) << "renaming with clangd";
                editorWidget->renameUsages(uiHeader, newName, cursor, callback);
            } else {
                qCDebug(log) << "renaming with built-in code model";
                snapshot.insert(cppDoc);
                snapshot.updateDependencyTable();
                CppEditor::CppModelManager::renameUsages(cppDoc, cursor, snapshot,
                                                         newName, callback);
            }
            return;
        }
    }
    reportRenamingError(oldName,
                        Designer::Tr::tr("Failed to locate corresponding symbol in ui header."));
}

void QtCreatorIntegration::slotSyncSettingsToDesigner()
{
    // Set promotion-relevant parameters on integration.
    setHeaderSuffix(CppEditor::preferredCxxHeaderSuffix(ProjectTree::currentProject()));
    setHeaderLowercase(FormClassWizardPage::lowercaseHeaderFiles());
}
