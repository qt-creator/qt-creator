// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppfindreferences.h"

#include "cppcodemodelsettings.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"
#include "cppworkingcopy.h"

#include <cplusplus/Overview.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/basefilefind.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>

#include <QtConcurrentMap>
#include <QCheckBox>
#include <QFutureWatcher>
#include <QVBoxLayout>

#include <functional>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

using namespace std::placeholders;

namespace CppEditor {

SearchResultColor::Style colorStyleForUsageType(CPlusPlus::Usage::Tags tags)
{
    if (tags.testFlag(CPlusPlus::Usage::Tag::Read))
        return SearchResultColor::Style::Alt1;
    if (tags.testAnyFlags({CPlusPlus::Usage::Tag::Write, CPlusPlus::Usage::Tag::WritableRef}))
        return SearchResultColor::Style::Alt2;
    return SearchResultColor::Style::Default;
}

QWidget *CppSearchResultFilter::createWidget()
{
    const auto widget = new QWidget;
    const auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    const auto readsCheckBox = new QCheckBox(Tr::tr("Reads"));
    readsCheckBox->setChecked(m_showReads);
    const auto writesCheckBox = new QCheckBox(Tr::tr("Writes"));
    writesCheckBox->setChecked(m_showWrites);
    const auto declsCheckBox = new QCheckBox(Tr::tr("Declarations"));
    declsCheckBox->setChecked(m_showDecls);
    const auto otherCheckBox = new QCheckBox(Tr::tr("Other"));
    otherCheckBox->setChecked(m_showOther);
    layout->addWidget(readsCheckBox);
    layout->addWidget(writesCheckBox);
    layout->addWidget(declsCheckBox);
    layout->addWidget(otherCheckBox);
    connect(readsCheckBox, &QCheckBox::toggled,
            this, [this](bool checked) { setValue(m_showReads, checked); });
    connect(writesCheckBox, &QCheckBox::toggled,
            this, [this](bool checked) { setValue(m_showWrites, checked); });
    connect(declsCheckBox, &QCheckBox::toggled,
            this, [this](bool checked) { setValue(m_showDecls, checked); });
    connect(otherCheckBox, &QCheckBox::toggled,
            this, [this](bool checked) { setValue(m_showOther, checked); });
    return widget;
}

bool CppSearchResultFilter::matches(const SearchResultItem &item) const
{
    const auto usageTags = CPlusPlus::Usage::Tags::fromInt(item.userData().toInt());
    if (usageTags.testFlag(CPlusPlus::Usage::Tag::Read))
        return m_showReads;
    if (usageTags.testAnyFlags({CPlusPlus::Usage::Tag::Write, CPlusPlus::Usage::Tag::WritableRef}))
        return m_showWrites;
    if (usageTags.testFlag(CPlusPlus::Usage::Tag::Declaration))
        return m_showDecls;
    return m_showOther;
}

void CppSearchResultFilter::setValue(bool &member, bool value)
{
    member = value;
    emit filterChanged();
}

namespace Internal {


static QByteArray getSource(const Utils::FilePath &fileName,
                            const WorkingCopy &workingCopy)
{
    if (const auto source = workingCopy.source(fileName)) {
        return *source;
    } else {
        QString fileContents;
        Utils::TextFileFormat format;
        QString error;
        QTextCodec *defaultCodec = EditorManager::defaultTextCodec();
        Utils::TextFileFormat::ReadResult result = Utils::TextFileFormat::readFile(
                    fileName, defaultCodec, &fileContents, &format, &error);
        if (result != Utils::TextFileFormat::ReadSuccess)
            qWarning() << "Could not read " << fileName << ". Error: " << error;

        return fileContents.toUtf8();
    }
}

static QByteArray typeId(CPlusPlus::Symbol *symbol)
{
    if (symbol->asEnum()) {
        return QByteArray("e");
    } else if (symbol->asFunction()) {
        return QByteArray("f");
    } else if (symbol->asNamespace()) {
        return QByteArray("n");
    } else if (symbol->asTemplate()) {
        return QByteArray("t");
    } else if (symbol->asNamespaceAlias()) {
        return QByteArray("na");
    } else if (symbol->asClass()) {
        return QByteArray("c");
    } else if (symbol->asBlock()) {
        return QByteArray("b");
    } else if (symbol->asUsingNamespaceDirective()) {
        return QByteArray("u");
    } else if (symbol->asUsingDeclaration()) {
        return QByteArray("ud");
    } else if (symbol->asDeclaration()) {
        QByteArray temp("d,");
        CPlusPlus::Overview pretty;
        temp.append(pretty.prettyType(symbol->type()).toUtf8());
        return temp;
    } else if (symbol->asArgument()) {
        return QByteArray("a");
    } else if (symbol->asTypenameArgument()) {
        return QByteArray("ta");
    } else if (symbol->asBaseClass()) {
        return QByteArray("bc");
    } else if (symbol->asForwardClassDeclaration()) {
        return QByteArray("fcd");
    } else if (symbol->asQtPropertyDeclaration()) {
        return QByteArray("qpd");
    } else if (symbol->asQtEnum()) {
        return QByteArray("qe");
    } else if (symbol->asObjCBaseClass()) {
        return QByteArray("ocbc");
    } else if (symbol->asObjCBaseProtocol()) {
        return QByteArray("ocbp");
    } else if (symbol->asObjCClass()) {
        return QByteArray("occ");
    } else if (symbol->asObjCForwardClassDeclaration()) {
        return QByteArray("ocfd");
    } else if (symbol->asObjCProtocol()) {
        return QByteArray("ocp");
    } else if (symbol->asObjCForwardProtocolDeclaration()) {
        return QByteArray("ocfpd");
    } else if (symbol->asObjCMethod()) {
        return QByteArray("ocm");
    } else if (symbol->asObjCPropertyDeclaration()) {
        return QByteArray("ocpd");
    }
    return QByteArray("unknown");
}

static QByteArray idForSymbol(CPlusPlus::Symbol *symbol)
{
    QByteArray uid(typeId(symbol));
    if (const CPlusPlus::Identifier *id = symbol->identifier()) {
        uid.append("|");
        uid.append(QByteArray(id->chars(), id->size()));
    } else if (CPlusPlus::Scope *scope = symbol->enclosingScope()) {
        // add the index of this symbol within its enclosing scope
        // (counting symbols without identifier of the same type)
        int count = 0;
        CPlusPlus::Scope::iterator it = scope->memberBegin();
        while (it != scope->memberEnd() && *it != symbol) {
            CPlusPlus::Symbol *val = *it;
            ++it;
            if (val->identifier() || typeId(val) != uid)
                continue;
            ++count;
        }
        uid.append(QString::number(count).toLocal8Bit());
    }
    return uid;
}

static QList<QByteArray> fullIdForSymbol(CPlusPlus::Symbol *symbol)
{
    QList<QByteArray> uid;
    CPlusPlus::Symbol *current = symbol;
    do {
        uid.prepend(idForSymbol(current));
        current = current->enclosingScope();
    } while (current);
    return uid;
}

namespace {

class ProcessFile
{
    const WorkingCopy workingCopy;
    const CPlusPlus::Snapshot snapshot;
    CPlusPlus::Document::Ptr symbolDocument;
    CPlusPlus::Symbol *symbol;
    QPromise<CPlusPlus::Usage> *m_promise;
    const bool categorize;

public:
    // needed by QtConcurrent
    using argument_type = const Utils::FilePath &;
    using result_type = QList<CPlusPlus::Usage>;

    ProcessFile(const WorkingCopy &workingCopy,
                const CPlusPlus::Snapshot snapshot,
                CPlusPlus::Document::Ptr symbolDocument,
                CPlusPlus::Symbol *symbol,
                QPromise<CPlusPlus::Usage> *promise,
                bool categorize)
        : workingCopy(workingCopy),
          snapshot(snapshot),
          symbolDocument(symbolDocument),
          symbol(symbol),
          m_promise(promise),
          categorize(categorize)
    { }

    QList<CPlusPlus::Usage> operator()(const Utils::FilePath &filePath)
    {
        QList<CPlusPlus::Usage> usages;
        m_promise->suspendIfRequested();
        if (m_promise->isCanceled())
            return usages;
        const CPlusPlus::Identifier *symbolId = symbol->identifier();

        if (CPlusPlus::Document::Ptr previousDoc = snapshot.document(filePath)) {
            CPlusPlus::Control *control = previousDoc->control();
            if (!control->findIdentifier(symbolId->chars(), symbolId->size()))
                return usages; // skip this document, it's not using symbolId.
        }
        CPlusPlus::Document::Ptr doc;
        const QByteArray unpreprocessedSource = getSource(filePath, workingCopy);

        if (symbolDocument && filePath == symbolDocument->filePath()) {
            doc = symbolDocument;
        } else {
            doc = snapshot.preprocessedDocument(unpreprocessedSource, filePath);
            doc->tokenize();
        }

        CPlusPlus::Control *control = doc->control();
        if (control->findIdentifier(symbolId->chars(), symbolId->size()) != nullptr) {
            if (doc != symbolDocument)
                doc->check();

            CPlusPlus::FindUsages process(unpreprocessedSource, doc, snapshot, categorize);
            process(symbol);

            usages = process.usages();
        }

        m_promise->suspendIfRequested();
        return usages;
    }
};

class UpdateUI
{
    QPromise<CPlusPlus::Usage> *m_promise;

public:
    explicit UpdateUI(QPromise<CPlusPlus::Usage> *promise): m_promise(promise) {}

    void operator()(QList<CPlusPlus::Usage> &, const QList<CPlusPlus::Usage> &usages)
    {
        for (const CPlusPlus::Usage &u : usages)
            m_promise->addResult(u);

        m_promise->setProgressValue(m_promise->future().progressValue() + 1);
    }
};

} // end of anonymous namespace

CppFindReferences::CppFindReferences(CppModelManager *modelManager)
    : QObject(modelManager),
      m_modelManager(modelManager)
{
}

CppFindReferences::~CppFindReferences() = default;

QList<int> CppFindReferences::references(CPlusPlus::Symbol *symbol,
                                         const CPlusPlus::LookupContext &context) const
{
    QList<int> references;

    CPlusPlus::FindUsages findUsages(context);
    findUsages(symbol);
    references = findUsages.references();

    return references;
}

static void find_helper(QPromise<CPlusPlus::Usage> &promise,
                        const WorkingCopy workingCopy,
                        const CPlusPlus::LookupContext &context,
                        CPlusPlus::Symbol *symbol,
                        bool categorize)
{
    const CPlusPlus::Identifier *symbolId = symbol->identifier();
    QTC_ASSERT(symbolId != nullptr, return);

    const CPlusPlus::Snapshot snapshot = context.snapshot();

    const FilePath sourceFile = symbol->filePath();
    FilePaths files{sourceFile};

    if (symbol->asClass()
        || symbol->asForwardClassDeclaration()
        || (symbol->enclosingScope()
            && !symbol->isStatic()
            && symbol->enclosingScope()->asNamespace())) {
        const CPlusPlus::Snapshot snapshotFromContext = context.snapshot();
        for (auto i = snapshotFromContext.begin(), ei = snapshotFromContext.end(); i != ei; ++i) {
            if (i.key() == sourceFile)
                continue;

            const CPlusPlus::Control *control = i.value()->control();

            if (control->findIdentifier(symbolId->chars(), symbolId->size()))
                files.append(i.key());
        }
    } else {
        files += snapshot.filesDependingOn(sourceFile);
    }
    files = Utils::filteredUnique(files);

    promise.setProgressRange(0, files.size());

    ProcessFile process(workingCopy, snapshot, context.thisDocument(), symbol, &promise, categorize);
    UpdateUI reduce(&promise);
    // This thread waits for blockingMappedReduced to finish, so reduce the pool's used thread count
    // so the blockingMappedReduced can use one more thread, and increase it again afterwards.
    QThreadPool::globalInstance()->releaseThread();
    QtConcurrent::blockingMappedReduced<QList<CPlusPlus::Usage> > (files, process, reduce);
    QThreadPool::globalInstance()->reserveThread();
    promise.setProgressValue(files.size());
}

void CppFindReferences::findUsages(CPlusPlus::Symbol *symbol,
                                   const CPlusPlus::LookupContext &context)
{
    findUsages(symbol, context, QString(), {}, false);
}

void CppFindReferences::findUsages(CPlusPlus::Symbol *symbol,
                                   const CPlusPlus::LookupContext &context,
                                   const QString &replacement,
                                   const std::function<void()> &callback,
                                   bool replace)
{
    CPlusPlus::Overview overview;
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(Tr::tr("C++ Usages:"),
                                                QString(),
                                                overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol)),
                                                replace ? SearchResultWindow::SearchAndReplace
                                                        : SearchResultWindow::SearchOnly,
                                                SearchResultWindow::PreserveCaseDisabled,
                                                QLatin1String("CppEditor"));
    search->setTextToReplace(replacement);
    if (callback)
        search->makeNonInteractive(callback);
    if (codeModelSettings()->categorizeFindReferences())
        search->setFilter(new CppSearchResultFilter);
    setupSearch(search);
    search->setSearchAgainSupported(true);
    connect(search, &SearchResult::searchAgainRequested, this,
            std::bind(&CppFindReferences::searchAgain, this, search));
    CppFindReferencesParameters parameters;
    parameters.symbolId = fullIdForSymbol(symbol);
    parameters.symbolFilePath = symbol->filePath();
    parameters.categorize = codeModelSettings()->categorizeFindReferences();
    parameters.preferLowerCaseFileNames = preferLowerCaseFileNames(
        ProjectManager::projectForFile(symbol->filePath()));

    if (symbol->asClass() || symbol->asForwardClassDeclaration()) {
        CPlusPlus::Overview overview;
        parameters.prettySymbolName =
                overview.prettyName(CPlusPlus::LookupContext::path(symbol).constLast());
    }

    search->setUserData(QVariant::fromValue(parameters));
    findAll_helper(search, symbol, context, codeModelSettings()->categorizeFindReferences());
}

void CppFindReferences::renameUsages(CPlusPlus::Symbol *symbol,
                                     const CPlusPlus::LookupContext &context,
                                     const QString &replacement,
                                     const std::function<void()> &callback)
{
    if (const CPlusPlus::Identifier *id = symbol->identifier()) {
        const QString textToReplace = replacement.isEmpty()
                ? QString::fromUtf8(id->chars(), id->size()) : replacement;
        findUsages(symbol, context, textToReplace, callback, true);
    }
}

void CppFindReferences::findAll_helper(SearchResult *search, CPlusPlus::Symbol *symbol,
                                       const CPlusPlus::LookupContext &context, bool categorize)
{
    if (!(symbol && symbol->identifier())) {
        search->finishSearch(false);
        return;
    }
    connect(search, &SearchResult::activated,
            [](const SearchResultItem& item) {
                Core::EditorManager::openEditorAtSearchResult(item);
            });

    if (search->isInteractive())
        SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
    const WorkingCopy workingCopy = CppModelManager::workingCopy();
    QFuture<CPlusPlus::Usage> result;
    result = Utils::asyncRun(CppModelManager::sharedThreadPool(), find_helper,
                             workingCopy, context, symbol, categorize);
    createWatcher(result, search);

    FutureProgress *progress = ProgressManager::addTask(result, Tr::tr("Searching for Usages"),
                                                        CppEditor::Constants::TASK_SEARCH);

    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void CppFindReferences::setupSearch(Core::SearchResult *search)
{
    auto renameFilesCheckBox = new QCheckBox();
    renameFilesCheckBox->setVisible(false);
    search->setAdditionalReplaceWidget(renameFilesCheckBox);
    connect(search, &SearchResult::replaceButtonClicked, this,
            std::bind(&CppFindReferences::onReplaceButtonClicked, this, search, _1, _2, _3));
}

void CppFindReferences::onReplaceButtonClicked(Core::SearchResult *search, const QString &text,
                                               const SearchResultItems &items, bool preserveCase)
{
    const Utils::FilePaths filePaths = TextEditor::BaseFileFind::replaceAll(text, items, preserveCase);
    if (!filePaths.isEmpty()) {
        m_modelManager->updateSourceFiles(Utils::toSet(filePaths));
        SearchResultWindow::instance()->hide();
    }

    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    if (parameters.filesToRename.isEmpty())
        return;

    auto renameFilesCheckBox = qobject_cast<QCheckBox *>(search->additionalReplaceWidget());
    if (!renameFilesCheckBox || !renameFilesCheckBox->isChecked())
        return;

    ProjectExplorerPlugin::renameFilesForSymbol(
                parameters.prettySymbolName, text, parameters.filesToRename,
                parameters.preferLowerCaseFileNames);
}

void CppFindReferences::searchAgain(SearchResult *search)
{
    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    parameters.filesToRename.clear();
    CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    search->restart();
    CPlusPlus::LookupContext context;
    CPlusPlus::Symbol *symbol = findSymbol(parameters, snapshot, &context);
    if (!symbol) {
        search->finishSearch(false);
        return;
    }
    findAll_helper(search, symbol, context, parameters.categorize);
}

namespace {
class UidSymbolFinder : public CPlusPlus::SymbolVisitor
{
public:
    explicit UidSymbolFinder(const QList<QByteArray> &uid) : m_uid(uid) { }
    CPlusPlus::Symbol *result() const { return m_result; }

    bool preVisit(CPlusPlus::Symbol *symbol) override
    {
        if (m_result)
            return false;
        int index = m_index;
        if (symbol->asScope())
            ++m_index;
        if (index >= m_uid.size())
            return false;
        if (idForSymbol(symbol) != m_uid.at(index))
            return false;
        if (index == m_uid.size() - 1) {
            // symbol found
            m_result = symbol;
            return false;
        }
        return true;
    }

    void postVisit(CPlusPlus::Symbol *symbol) override
    {
        if (symbol->asScope())
            --m_index;
    }

private:
    QList<QByteArray> m_uid;
    int m_index = 0;
    CPlusPlus::Symbol *m_result = nullptr;
};
}

CPlusPlus::Symbol *CppFindReferences::findSymbol(const CppFindReferencesParameters &parameters,
                                                 const CPlusPlus::Snapshot &snapshot,
                                                 CPlusPlus::LookupContext *context)
{
    QTC_ASSERT(context, return nullptr);
    if (!snapshot.contains(parameters.symbolFilePath))
        return nullptr;

    CPlusPlus::Document::Ptr newSymbolDocument = snapshot.document(parameters.symbolFilePath);
    // document is not parsed and has no bindings yet, do it
    QByteArray source = getSource(newSymbolDocument->filePath(), CppModelManager::workingCopy());
    CPlusPlus::Document::Ptr doc =
            snapshot.preprocessedDocument(source, newSymbolDocument->filePath());
    doc->check();

    // find matching symbol in new document and return the new parameters
    UidSymbolFinder finder(parameters.symbolId);
    finder.accept(doc->globalNamespace());
    if (finder.result()) {
        *context = CPlusPlus::LookupContext(doc, snapshot);
        return finder.result();
    }
    return nullptr;
}

static void displayResults(SearchResult *search,
                           QFutureWatcher<CPlusPlus::Usage> *watcher,
                           int first,
                           int last)
{
    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();

    for (int index = first; index != last; ++index) {
        const CPlusPlus::Usage result = watcher->future().resultAt(index);
        SearchResultItem item;
        item.setFilePath(result.path);
        item.setMainRange(result.line, result.col, result.len);
        item.setLineText(result.lineText);
        item.setUserData(result.tags.toInt());
        item.setContainingFunctionName(result.containingFunction);
        item.setStyle(colorStyleForUsageType(result.tags));
        item.setUseTextEditorFont(true);
        if (search->supportsReplace()) {
            const Node * const node = ProjectTree::nodeForFile(result.path);
            item.setSelectForReplacement(!ProjectManager::hasProjects()
                                         || (node && !node->isGenerated()));
        }
        search->addResult(item);

        if (parameters.prettySymbolName.isEmpty())
            continue;

        if (parameters.filesToRename.contains(result.path))
            continue;

        if (!ProjectManager::projectForFile(result.path))
            continue;

        if (result.path.baseName().compare(parameters.prettySymbolName, Qt::CaseInsensitive) == 0)
            parameters.filesToRename.append(result.path);
    }

    search->setUserData(QVariant::fromValue(parameters));
}

static void searchFinished(SearchResult *search, QFutureWatcher<CPlusPlus::Usage> *watcher)
{
    if (!watcher->isCanceled() && search->supportsReplace()) {
        search->addResults(symbolOccurrencesInDeclarationComments(search->allItems()),
                           SearchResult::AddSortedByPosition);
    }
    search->finishSearch(watcher->isCanceled());

    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    if (!parameters.filesToRename.isEmpty()) {
        const QStringList filesToRename
                = Utils::transform<QList>(parameters.filesToRename, &FilePath::toUserOutput);

        auto renameCheckBox = qobject_cast<QCheckBox *>(search->additionalReplaceWidget());
        if (renameCheckBox) {
            renameCheckBox->setText(Tr::tr("Re&name %n files", nullptr, filesToRename.size()));
            renameCheckBox->setToolTip(Tr::tr("Files:\n%1").arg(filesToRename.join('\n')));
            renameCheckBox->setVisible(true);
        }
    }

    watcher->deleteLater();
}

namespace {

class FindMacroUsesInFile
{
    const WorkingCopy workingCopy;
    const CPlusPlus::Snapshot snapshot;
    const CPlusPlus::Macro &macro;
    QPromise<CPlusPlus::Usage> *m_promise;

public:
    // needed by QtConcurrent
    using argument_type = const Utils::FilePath &;
    using result_type = QList<CPlusPlus::Usage>;

    FindMacroUsesInFile(const WorkingCopy &workingCopy,
                        const CPlusPlus::Snapshot snapshot,
                        const CPlusPlus::Macro &macro,
                        QPromise<CPlusPlus::Usage> *promise)
        : workingCopy(workingCopy), snapshot(snapshot), macro(macro), m_promise(promise)
    { }

    QList<CPlusPlus::Usage> operator()(const Utils::FilePath &fileName)
    {
        QList<CPlusPlus::Usage> usages;
        CPlusPlus::Document::Ptr doc = snapshot.document(fileName);
        QByteArray source;

restart_search:
        m_promise->suspendIfRequested();
        if (m_promise->isCanceled())
            return usages;

        usages.clear();
        for (const CPlusPlus::Document::MacroUse &use : doc->macroUses()) {
            const CPlusPlus::Macro &useMacro = use.macro();

            if (useMacro.filePath() == macro.filePath()) { // Check if this is a match, but possibly against an outdated document.
                if (source.isEmpty())
                    source = getSource(fileName, workingCopy);

                if (macro.fileRevision() > useMacro.fileRevision()) {
                    // yes, it is outdated, so re-preprocess and start from scratch for this file.
                    doc = snapshot.preprocessedDocument(source, fileName);
                    usages.clear();
                    goto restart_search;
                }

                if (macro.name() == useMacro.name()) {
                    unsigned column;
                    const QString &lineSource = matchingLine(use.bytesBegin(), source, &column);
                    usages.append(CPlusPlus::Usage(fileName, lineSource, {},
                                                   {}, use.beginLine(),
                                                   column, useMacro.nameToQString().size()));
                }
            }
        }

        m_promise->suspendIfRequested();
        return usages;
    }

    static QString matchingLine(unsigned bytesOffsetOfUseStart, const QByteArray &utf8Source,
                                unsigned *columnOfUseStart = nullptr)
    {
        int lineBegin = utf8Source.lastIndexOf('\n', bytesOffsetOfUseStart) + 1;
        int lineEnd = utf8Source.indexOf('\n', bytesOffsetOfUseStart);
        if (lineEnd == -1)
            lineEnd = utf8Source.length();

        if (columnOfUseStart) {
            *columnOfUseStart = 0;
            const char *startOfUse = utf8Source.constData() + bytesOffsetOfUseStart;
            QTC_ASSERT(startOfUse < utf8Source.constData() + lineEnd, return QString());
            const char *currentSourceByte = utf8Source.constData() + lineBegin;
            unsigned char yychar = *currentSourceByte;
            while (currentSourceByte != startOfUse)
                CPlusPlus::Lexer::yyinp_utf8(currentSourceByte, yychar, *columnOfUseStart);
        }

        const QByteArray matchingLine = utf8Source.mid(lineBegin, lineEnd - lineBegin);
        return QString::fromUtf8(matchingLine, matchingLine.size());
    }
};

} // end of anonymous namespace

static void findMacroUses_helper(QPromise<CPlusPlus::Usage> &promise,
                                 const WorkingCopy workingCopy,
                                 const CPlusPlus::Snapshot snapshot,
                                 const CPlusPlus::Macro macro)
{
    const FilePath sourceFile = macro.filePath();
    FilePaths files{sourceFile};
    files = Utils::filteredUnique(files + snapshot.filesDependingOn(sourceFile));

    promise.setProgressRange(0, files.size());
    FindMacroUsesInFile process(workingCopy, snapshot, macro, &promise);
    UpdateUI reduce(&promise);
    // This thread waits for blockingMappedReduced to finish, so reduce the pool's used thread count
    // so the blockingMappedReduced can use one more thread, and increase it again afterwards.
    QThreadPool::globalInstance()->releaseThread();
    QtConcurrent::blockingMappedReduced<QList<CPlusPlus::Usage> > (files, process, reduce);
    QThreadPool::globalInstance()->reserveThread();
    promise.setProgressValue(files.size());
}

void CppFindReferences::findMacroUses(const CPlusPlus::Macro &macro)
{
    findMacroUses(macro, QString(), false);
}

void CppFindReferences::findMacroUses(const CPlusPlus::Macro &macro, const QString &replacement,
                                      bool replace)
{
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(
                Tr::tr("C++ Macro Usages:"),
                QString(),
                macro.nameToQString(),
                replace ? SearchResultWindow::SearchAndReplace
                        : SearchResultWindow::SearchOnly,
                SearchResultWindow::PreserveCaseDisabled,
                QLatin1String("CppEditor"));

    search->setTextToReplace(replacement);
    setupSearch(search);
    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);

    connect(search, &SearchResult::activated, search, [](const SearchResultItem &item) {
        Core::EditorManager::openEditorAtSearchResult(item);
    });

    const CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    const WorkingCopy workingCopy = CppModelManager::workingCopy();

    // add the macro definition itself
    {
        const QByteArray &source = getSource(macro.filePath(), workingCopy);
        unsigned column;
        const QString line = FindMacroUsesInFile::matchingLine(macro.bytesOffset(), source,
                                                               &column);
        SearchResultItem item;
        const FilePath filePath = macro.filePath();
        item.setFilePath(filePath);
        item.setLineText(line);
        item.setMainRange(macro.line(), column, macro.nameToQString().length());
        item.setUseTextEditorFont(true);
        if (search->supportsReplace())
            item.setSelectForReplacement(ProjectManager::projectForFile(filePath));
        search->addResult(item);
    }

    QFuture<CPlusPlus::Usage> result;
    result = Utils::asyncRun(CppModelManager::sharedThreadPool(), findMacroUses_helper,
                             workingCopy, snapshot, macro);
    createWatcher(result, search);

    FutureProgress *progress = ProgressManager::addTask(result, Tr::tr("Searching for Usages"),
                                                        CppEditor::Constants::TASK_SEARCH);
    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void CppFindReferences::renameMacroUses(const CPlusPlus::Macro &macro, const QString &replacement)
{
    const QString textToReplace = replacement.isEmpty() ? macro.nameToQString() : replacement;
    findMacroUses(macro, textToReplace, true);
}

void CppFindReferences::checkUnused(Core::SearchResult *search, const Link &link,
                                    CPlusPlus::Symbol *symbol,
                                    const CPlusPlus::LookupContext &context,
                                    const LinkHandler &callback)
{
    const auto isProperUsage = [symbol](const CPlusPlus::Usage &usage) {
        if (!usage.tags.testFlag(CPlusPlus::Usage::Tag::Declaration))
            return usage.containingFunctionSymbol != symbol;
        return usage.tags.testAnyFlags({CPlusPlus::Usage::Tag::Override,
                                        CPlusPlus::Usage::Tag::MocInvokable,
                                        CPlusPlus::Usage::Tag::Template,
                                        CPlusPlus::Usage::Tag::Operator,
                                        CPlusPlus::Usage::Tag::ConstructorDestructor});
    };
    const auto watcher = new QFutureWatcher<CPlusPlus::Usage>();
    connect(watcher, &QFutureWatcherBase::finished, watcher,
            [watcher, link, callback, search, isProperUsage] {
        watcher->deleteLater();
        if (watcher->isCanceled())
            return callback(link);
        for (int i = 0; i < watcher->future().resultCount(); ++i) {
            if (isProperUsage(watcher->resultAt(i)))
                return callback(link);
        }
        for (int i = 0; i < watcher->future().resultCount(); ++i) {
            const CPlusPlus::Usage usage = watcher->resultAt(i);
            SearchResultItem item;
            item.setFilePath(usage.path);
            item.setLineText(usage.lineText);
            item.setMainRange(usage.line, usage.col, usage.len);
            item.setUseTextEditorFont(true);
            search->addResult(item);
        }
        callback(link);
    });
    connect(watcher, &QFutureWatcherBase::resultsReadyAt, search,
            [watcher, isProperUsage](int first, int end) {
        for (int i = first; i < end; ++i) {
            if (isProperUsage(watcher->resultAt(i))) {
                watcher->cancel();
                break;
            }
        }
    });
    connect(search, &SearchResult::canceled, watcher, [watcher] { watcher->cancel(); });
    connect(search, &SearchResult::destroyed, watcher, [watcher] { watcher->cancel(); });
    watcher->setFuture(Utils::asyncRun(CppModelManager::sharedThreadPool(), find_helper,
                                       CppModelManager::workingCopy(), context, symbol, true));
}

void CppFindReferences::createWatcher(const QFuture<CPlusPlus::Usage> &future, SearchResult *search)
{
    auto watcher = new QFutureWatcher<CPlusPlus::Usage>();
    // auto-delete:
    connect(watcher, &QFutureWatcherBase::finished, watcher, [search, watcher]() {
                searchFinished(search, watcher);
            });

    connect(watcher, &QFutureWatcherBase::resultsReadyAt, search,
            [search, watcher](int first, int last) {
                displayResults(search, watcher, first, last);
            });
    connect(watcher, &QFutureWatcherBase::finished, search, [search, watcher]() {
        search->finishSearch(watcher->isCanceled());
    });
    connect(search, &SearchResult::canceled, watcher, [watcher]() { watcher->cancel(); });
    connect(search, &SearchResult::paused, watcher, [watcher](bool paused) {
        if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
            watcher->setPaused(paused);
    });
    watcher->setPendingResultsLimit(1);
    watcher->setFuture(future);
}

} // namespace Internal
} // namespace CppEditor
