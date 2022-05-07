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

#include "cppfindreferences.h"

#include "cppcodemodelsettings.h"
#include "cppeditorconstants.h"
#include "cppfilesettingspage.h"
#include "cppmodelmanager.h"
#include "cpptoolsreuse.h"
#include "cppworkingcopy.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/session.h>
#include <texteditor/basefilefind.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/textfileformat.h>

#include <cplusplus/Overview.h>
#include <QtConcurrentMap>
#include <QCheckBox>
#include <QDir>
#include <QFutureWatcher>
#include <QVBoxLayout>

#include <functional>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

namespace { static bool isAllLowerCase(const QString &text) { return text.toLower() == text; } }

SearchResultColor::Style colorStyleForUsageType(CPlusPlus::Usage::Type type)
{
    switch (type) {
    case CPlusPlus::Usage::Type::Read:
        return SearchResultColor::Style::Alt1;
    case CPlusPlus::Usage::Type::Initialization:
    case CPlusPlus::Usage::Type::Write:
    case CPlusPlus::Usage::Type::WritableRef:
        return SearchResultColor::Style::Alt2;
    case CPlusPlus::Usage::Type::Declaration:
    case CPlusPlus::Usage::Type::Other:
        return SearchResultColor::Style::Default;
    }
    return SearchResultColor::Style::Default; // For dumb compilers.
}

void renameFilesForSymbol(const QString &oldSymbolName, const QString &newSymbolName,
                          const QVector<Node *> &files)
{
    Internal::CppFileSettings settings;
    settings.fromSettings(Core::ICore::settings());

    const QStringList newPaths =
            Utils::transform<QList>(files,
                                    [&oldSymbolName, newSymbolName, &settings](const Node *node) -> QString {
        const QFileInfo fi = node->filePath().toFileInfo();
        const QString oldBaseName = fi.baseName();
        QString newBaseName = newSymbolName;

        // 1) new symbol lowercase: new base name lowercase
        if (isAllLowerCase(newSymbolName)) {
            newBaseName = newSymbolName;

        // 2) old base name mixed case: new base name is verbatim symbol name
        } else if (!isAllLowerCase(oldBaseName)) {
            newBaseName = newSymbolName;

        // 3) old base name lowercase, old symbol mixed case: new base name lowercase
        } else if (!isAllLowerCase(oldSymbolName)) {
            newBaseName = newSymbolName.toLower();

        // 4) old base name lowercase, old symbol lowercase, new symbol mixed case:
        //    use the preferences setting for new base name case
        } else if (settings.lowerCaseFiles) {
            newBaseName = newSymbolName.toLower();
        }

        if (newBaseName == oldBaseName)
            return QString();

        return fi.absolutePath() + "/" + newBaseName + '.' + fi.completeSuffix();
    });

    for (int i = 0; i < files.size(); ++i) {
        if (!newPaths.at(i).isEmpty()) {
            Node *node = files.at(i);
            ProjectExplorerPlugin::renameFile(node, newPaths.at(i));
        }
    }
}

QString functionAt(CPlusPlus::Document::Ptr doc,
                   int line,
                   int column,
                   int *lineOpeningDeclaratorParenthesis,
                   int *columnOut)
{
    using namespace CPlusPlus;
    if (line < 1 || column < 1)
        return QString();

    Symbol *symbol = doc->lastVisibleSymbolAt(line, column);
    if (!symbol)
        return QString();

    // Find the enclosing function scope (which might be several levels up, or we might be standing
    // on it)
    Scope *scope = symbol->asScope();
    if (!scope)
        scope = symbol->enclosingScope();

    while (scope && !scope->isFunction())
        scope = scope->enclosingScope();

    if (!scope)
        return QString();

    // We found the function scope
    if (lineOpeningDeclaratorParenthesis)
        doc->translationUnit()->getPosition(scope->startOffset(),
                                            lineOpeningDeclaratorParenthesis,
                                            columnOut);

    const QList<const Name *> fullyQualifiedName = LookupContext::fullyQualifiedName(scope);
    return Overview().prettyName(fullyQualifiedName);
}

QWidget *CppSearchResultFilter::createWidget()
{
    const auto widget = new QWidget;
    const auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    const auto readsCheckBox = new QCheckBox(Internal::CppFindReferences::tr("Reads"));
    readsCheckBox->setChecked(m_showReads);
    const auto writesCheckBox = new QCheckBox(Internal::CppFindReferences::tr("Writes"));
    writesCheckBox->setChecked(m_showWrites);
    const auto declsCheckBox = new QCheckBox(Internal::CppFindReferences::tr("Declarations"));
    declsCheckBox->setChecked(m_showDecls);
    const auto otherCheckBox = new QCheckBox(Internal::CppFindReferences::tr("Other"));
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
    switch (static_cast<CPlusPlus::Usage::Type>(item.userData().toInt())) {
    case CPlusPlus::Usage::Type::Read:
        return m_showReads;
    case CPlusPlus::Usage::Type::Write:
    case CPlusPlus::Usage::Type::WritableRef:
    case CPlusPlus::Usage::Type::Initialization:
        return m_showWrites;
    case CPlusPlus::Usage::Type::Declaration:
        return m_showDecls;
    case CPlusPlus::Usage::Type::Other:
        return m_showOther;
    }
    return false;
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
    if (workingCopy.contains(fileName)) {
        return workingCopy.source(fileName);
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
    QFutureInterface<CPlusPlus::Usage> *future;
    const bool categorize;

public:
    // needed by QtConcurrent
    using argument_type = const Utils::FilePath &;
    using result_type = QList<CPlusPlus::Usage>;

    ProcessFile(const WorkingCopy &workingCopy,
                const CPlusPlus::Snapshot snapshot,
                CPlusPlus::Document::Ptr symbolDocument,
                CPlusPlus::Symbol *symbol,
                QFutureInterface<CPlusPlus::Usage> *future,
                bool categorize)
        : workingCopy(workingCopy),
          snapshot(snapshot),
          symbolDocument(symbolDocument),
          symbol(symbol),
          future(future),
          categorize(categorize)
    { }

    QList<CPlusPlus::Usage> operator()(const Utils::FilePath &fileName)
    {
        QList<CPlusPlus::Usage> usages;
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return usages;
        const CPlusPlus::Identifier *symbolId = symbol->identifier();

        if (CPlusPlus::Document::Ptr previousDoc = snapshot.document(fileName)) {
            CPlusPlus::Control *control = previousDoc->control();
            if (!control->findIdentifier(symbolId->chars(), symbolId->size()))
                return usages; // skip this document, it's not using symbolId.
        }
        CPlusPlus::Document::Ptr doc;
        const QByteArray unpreprocessedSource = getSource(fileName, workingCopy);

        if (symbolDocument && fileName == Utils::FilePath::fromString(symbolDocument->fileName())) {
            doc = symbolDocument;
        } else {
            doc = snapshot.preprocessedDocument(unpreprocessedSource, fileName);
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

        if (future->isPaused())
            future->waitForResume();
        return usages;
    }
};

class UpdateUI
{
    QFutureInterface<CPlusPlus::Usage> *future;

public:
    explicit UpdateUI(QFutureInterface<CPlusPlus::Usage> *future): future(future) {}

    void operator()(QList<CPlusPlus::Usage> &, const QList<CPlusPlus::Usage> &usages)
    {
        foreach (const CPlusPlus::Usage &u, usages)
            future->reportResult(u);

        future->setProgressValue(future->progressValue() + 1);
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

static void find_helper(QFutureInterface<CPlusPlus::Usage> &future,
                        const WorkingCopy workingCopy,
                        const CPlusPlus::LookupContext &context,
                        CPlusPlus::Symbol *symbol,
                        bool categorize)
{
    const CPlusPlus::Identifier *symbolId = symbol->identifier();
    QTC_ASSERT(symbolId != nullptr, return);

    const CPlusPlus::Snapshot snapshot = context.snapshot();

    const Utils::FilePath sourceFile = Utils::FilePath::fromUtf8(symbol->fileName(),
                                                                 symbol->fileNameLength());
    Utils::FilePaths files{sourceFile};

    if (symbol->isClass()
        || symbol->isForwardClassDeclaration()
        || (symbol->enclosingScope()
            && !symbol->isStatic()
            && symbol->enclosingScope()->isNamespace())) {
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

    future.setProgressRange(0, files.size());

    ProcessFile process(workingCopy, snapshot, context.thisDocument(), symbol, &future, categorize);
    UpdateUI reduce(&future);
    // This thread waits for blockingMappedReduced to finish, so reduce the pool's used thread count
    // so the blockingMappedReduced can use one more thread, and increase it again afterwards.
    QThreadPool::globalInstance()->releaseThread();
    QtConcurrent::blockingMappedReduced<QList<CPlusPlus::Usage> > (files, process, reduce);
    QThreadPool::globalInstance()->reserveThread();
    future.setProgressValue(files.size());
}

void CppFindReferences::findUsages(CPlusPlus::Symbol *symbol,
                                   const CPlusPlus::LookupContext &context)
{
    findUsages(symbol, context, QString(), false);
}

void CppFindReferences::findUsages(CPlusPlus::Symbol *symbol,
                                   const CPlusPlus::LookupContext &context,
                                   const QString &replacement,
                                   bool replace)
{
    CPlusPlus::Overview overview;
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(tr("C++ Usages:"),
                                                QString(),
                                                overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol)),
                                                replace ? SearchResultWindow::SearchAndReplace
                                                        : SearchResultWindow::SearchOnly,
                                                SearchResultWindow::PreserveCaseDisabled,
                                                QLatin1String("CppEditor"));
    search->setTextToReplace(replacement);
    if (codeModelSettings()->categorizeFindReferences())
        search->setFilter(new CppSearchResultFilter);
    auto renameFilesCheckBox = new QCheckBox();
    renameFilesCheckBox->setVisible(false);
    search->setAdditionalReplaceWidget(renameFilesCheckBox);
    connect(search, &SearchResult::replaceButtonClicked,
            this, &CppFindReferences::onReplaceButtonClicked);
    search->setSearchAgainSupported(true);
    connect(search, &SearchResult::searchAgainRequested, this, &CppFindReferences::searchAgain);
    CppFindReferencesParameters parameters;
    parameters.symbolId = fullIdForSymbol(symbol);
    parameters.symbolFileName = QByteArray(symbol->fileName());
    parameters.categorize = codeModelSettings()->categorizeFindReferences();

    if (symbol->isClass() || symbol->isForwardClassDeclaration()) {
        CPlusPlus::Overview overview;
        parameters.prettySymbolName =
                overview.prettyName(CPlusPlus::LookupContext::path(symbol).constLast());
    }

    search->setUserData(QVariant::fromValue(parameters));
    findAll_helper(search, symbol, context, codeModelSettings()->categorizeFindReferences());
}

void CppFindReferences::renameUsages(CPlusPlus::Symbol *symbol,
                                     const CPlusPlus::LookupContext &context,
                                     const QString &replacement)
{
    if (const CPlusPlus::Identifier *id = symbol->identifier()) {
        const QString textToReplace = replacement.isEmpty()
                ? QString::fromUtf8(id->chars(), id->size()) : replacement;
        findUsages(symbol, context, textToReplace, true);
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

    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);
    const WorkingCopy workingCopy = m_modelManager->workingCopy();
    QFuture<CPlusPlus::Usage> result;
    result = Utils::runAsync(m_modelManager->sharedThreadPool(), find_helper,
                             workingCopy, context, symbol, categorize);
    createWatcher(result, search);

    FutureProgress *progress = ProgressManager::addTask(result, tr("Searching for Usages"),
                                                              CppEditor::Constants::TASK_SEARCH);

    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void CppFindReferences::onReplaceButtonClicked(const QString &text,
                                               const QList<SearchResultItem> &items,
                                               bool preserveCase)
{
    const Utils::FilePaths filePaths = TextEditor::BaseFileFind::replaceAll(text, items, preserveCase);
    if (!filePaths.isEmpty()) {
        m_modelManager->updateSourceFiles(
            Utils::transform<QSet>(filePaths, &Utils::FilePath::toString));
        SearchResultWindow::instance()->hide();
    }

    auto search = qobject_cast<SearchResult *>(sender());
    QTC_ASSERT(search, return);

    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    if (parameters.filesToRename.isEmpty())
        return;

    auto renameFilesCheckBox = qobject_cast<QCheckBox *>(search->additionalReplaceWidget());
    if (!renameFilesCheckBox || !renameFilesCheckBox->isChecked())
        return;

    renameFilesForSymbol(parameters.prettySymbolName, text, parameters.filesToRename);
}

void CppFindReferences::searchAgain()
{
    auto search = qobject_cast<SearchResult *>(sender());
    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    parameters.filesToRename.clear();
    CPlusPlus::Snapshot snapshot = CppModelManager::instance()->snapshot();
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
    QString symbolFile = QLatin1String(parameters.symbolFileName);
    if (!snapshot.contains(symbolFile))
        return nullptr;

    CPlusPlus::Document::Ptr newSymbolDocument = snapshot.document(symbolFile);
    // document is not parsed and has no bindings yet, do it
    QByteArray source = getSource(Utils::FilePath::fromString(newSymbolDocument->fileName()),
                                  m_modelManager->workingCopy());
    CPlusPlus::Document::Ptr doc =
            snapshot.preprocessedDocument(source, FilePath::fromString(newSymbolDocument->fileName()));
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

static void displayResults(SearchResult *search, QFutureWatcher<CPlusPlus::Usage> *watcher,
                           int first, int last)
{
    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();

    for (int index = first; index != last; ++index) {
        const CPlusPlus::Usage result = watcher->future().resultAt(index);
        SearchResultItem item;
        item.setFilePath(result.path);
        item.setMainRange(result.line, result.col, result.len);
        item.setLineText(result.lineText);
        item.setUserData(int(result.type));
        item.setStyle(colorStyleForUsageType(result.type));
        item.setUseTextEditorFont(true);
        if (search->supportsReplace())
            item.setSelectForReplacement(SessionManager::projectForFile(result.path));

        const CPlusPlus::Snapshot snapshot = CppModelManager::instance()->snapshot();
        auto document = snapshot.document(result.path);
        int lineOpeningDeclaratorParenthesis;
        // In case we're looking for a function, need to look in the middle of the declaration -
        // otherwise the previous function might be returned.
        int col = (result.col + result.len) / 2;

        int functionColumn{-1};
        auto func = functionAt(document,
                               result.line,
                               col,
                               &lineOpeningDeclaratorParenthesis,
                               &functionColumn);

        QSharedPointer<SearchResultItem> callingFunctionItem{new SearchResultItem};

        const QString noCallingFunctionName{"No calling function"};
        QString functionName{noCallingFunctionName};
        int functionLine{item.mainRange().begin.line};
        int functionNameLength{noCallingFunctionName.size()};

        if (func.size()) {
            functionName = func;
            functionNameLength = func.length();
            functionLine = lineOpeningDeclaratorParenthesis;
        }

        callingFunctionItem->setLineText(functionName);
        callingFunctionItem->setPath(item.path());
        callingFunctionItem->setMainRange(functionLine, 0, functionNameLength);
        callingFunctionItem->setUseTextEditorFont(true);

        item.setCallingFunctionItem(callingFunctionItem);
        search->addResult(item);

        if (parameters.prettySymbolName.isEmpty())
            continue;

        if (Utils::contains(parameters.filesToRename, Utils::equal(&Node::filePath, result.path)))
            continue;

        Node *node = ProjectTree::nodeForFile(result.path);
        if (!node) // Not part of any project
            continue;

        const QFileInfo fi = node->filePath().toFileInfo();
        if (fi.baseName().compare(parameters.prettySymbolName, Qt::CaseInsensitive) == 0)
            parameters.filesToRename.append(node);
    }

    search->setUserData(QVariant::fromValue(parameters));
}

static void searchFinished(SearchResult *search, QFutureWatcher<CPlusPlus::Usage> *watcher)
{
    search->finishSearch(watcher->isCanceled());

    CppFindReferencesParameters parameters = search->userData().value<CppFindReferencesParameters>();
    if (!parameters.filesToRename.isEmpty()) {
        const QStringList filesToRename
                = Utils::transform<QList>(parameters.filesToRename, [](const Node *node) {
            return node->filePath().toUserOutput();
        });

        auto renameCheckBox = qobject_cast<QCheckBox *>(search->additionalReplaceWidget());
        if (renameCheckBox) {
            renameCheckBox->setText(CppFindReferences::tr("Re&name %n files", nullptr, filesToRename.size()));
            renameCheckBox->setToolTip(CppFindReferences::tr("Files:\n%1").arg(filesToRename.join('\n')));
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
    QFutureInterface<CPlusPlus::Usage> *future;

public:
    // needed by QtConcurrent
    using argument_type = const Utils::FilePath &;
    using result_type = QList<CPlusPlus::Usage>;

    FindMacroUsesInFile(const WorkingCopy &workingCopy,
                        const CPlusPlus::Snapshot snapshot,
                        const CPlusPlus::Macro &macro,
                        QFutureInterface<CPlusPlus::Usage> *future)
        : workingCopy(workingCopy), snapshot(snapshot), macro(macro), future(future)
    { }

    QList<CPlusPlus::Usage> operator()(const Utils::FilePath &fileName)
    {
        QList<CPlusPlus::Usage> usages;
        CPlusPlus::Document::Ptr doc = snapshot.document(fileName);
        QByteArray source;

restart_search:
        if (future->isPaused())
            future->waitForResume();
        if (future->isCanceled())
            return usages;

        usages.clear();
        foreach (const CPlusPlus::Document::MacroUse &use, doc->macroUses()) {
            const CPlusPlus::Macro &useMacro = use.macro();

            if (useMacro.fileName() == macro.fileName()) { // Check if this is a match, but possibly against an outdated document.
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
                    usages.append(CPlusPlus::Usage(fileName, lineSource,
                                                   CPlusPlus::Usage::Type::Other, use.beginLine(),
                                                   column, useMacro.nameToQString().size()));
                }
            }
        }

        if (future->isPaused())
            future->waitForResume();
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

static void findMacroUses_helper(QFutureInterface<CPlusPlus::Usage> &future,
                                 const WorkingCopy workingCopy,
                                 const CPlusPlus::Snapshot snapshot,
                                 const CPlusPlus::Macro macro)
{
    const Utils::FilePath sourceFile = Utils::FilePath::fromString(macro.fileName());
    Utils::FilePaths files{sourceFile};
    files = Utils::filteredUnique(files + snapshot.filesDependingOn(sourceFile));

    future.setProgressRange(0, files.size());
    FindMacroUsesInFile process(workingCopy, snapshot, macro, &future);
    UpdateUI reduce(&future);
    // This thread waits for blockingMappedReduced to finish, so reduce the pool's used thread count
    // so the blockingMappedReduced can use one more thread, and increase it again afterwards.
    QThreadPool::globalInstance()->releaseThread();
    QtConcurrent::blockingMappedReduced<QList<CPlusPlus::Usage> > (files, process, reduce);
    QThreadPool::globalInstance()->reserveThread();
    future.setProgressValue(files.size());
}

void CppFindReferences::findMacroUses(const CPlusPlus::Macro &macro)
{
    findMacroUses(macro, QString(), false);
}

void CppFindReferences::findMacroUses(const CPlusPlus::Macro &macro, const QString &replacement,
                                      bool replace)
{
    SearchResult *search = SearchResultWindow::instance()->startNewSearch(
                tr("C++ Macro Usages:"),
                QString(),
                macro.nameToQString(),
                replace ? SearchResultWindow::SearchAndReplace
                        : SearchResultWindow::SearchOnly,
                SearchResultWindow::PreserveCaseDisabled,
                QLatin1String("CppEditor"));

    search->setTextToReplace(replacement);
    auto renameFilesCheckBox = new QCheckBox();
    renameFilesCheckBox->setVisible(false);
    search->setAdditionalReplaceWidget(renameFilesCheckBox);
    connect(search, &SearchResult::replaceButtonClicked,
            this, &CppFindReferences::onReplaceButtonClicked);

    SearchResultWindow::instance()->popup(IOutputPane::ModeSwitch | IOutputPane::WithFocus);

    connect(search, &SearchResult::activated,
            [](const Core::SearchResultItem& item) {
                Core::EditorManager::openEditorAtSearchResult(item);
            });

    const CPlusPlus::Snapshot snapshot = m_modelManager->snapshot();
    const WorkingCopy workingCopy = m_modelManager->workingCopy();

    // add the macro definition itself
    {
        const QByteArray &source = getSource(Utils::FilePath::fromString(macro.fileName()),
                                             workingCopy);
        unsigned column;
        const QString line = FindMacroUsesInFile::matchingLine(macro.bytesOffset(), source,
                                                               &column);
        SearchResultItem item;
        const Utils::FilePath filePath = Utils::FilePath::fromString(macro.fileName());
        item.setFilePath(filePath);
        item.setLineText(line);
        item.setMainRange(macro.line(), column, macro.nameToQString().length());
        item.setUseTextEditorFont(true);
        if (search->supportsReplace())
            item.setSelectForReplacement(SessionManager::projectForFile(filePath));
        search->addResult(item);
    }

    QFuture<CPlusPlus::Usage> result;
    result = Utils::runAsync(m_modelManager->sharedThreadPool(), findMacroUses_helper,
                             workingCopy, snapshot, macro);
    createWatcher(result, search);

    FutureProgress *progress = ProgressManager::addTask(result, tr("Searching for Usages"),
                                                              CppEditor::Constants::TASK_SEARCH);
    connect(progress, &FutureProgress::clicked, search, &SearchResult::popup);
}

void CppFindReferences::renameMacroUses(const CPlusPlus::Macro &macro, const QString &replacement)
{
    const QString textToReplace = replacement.isEmpty() ? macro.nameToQString() : replacement;
    findMacroUses(macro, textToReplace, true);
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
    connect(search, &SearchResult::cancelled, watcher, [watcher]() { watcher->cancel(); });
    connect(search, &SearchResult::paused, watcher, [watcher](bool paused) {
        if (!paused || watcher->isRunning()) // guard against pausing when the search is finished
            watcher->setPaused(paused);
    });
    watcher->setPendingResultsLimit(1);
    watcher->setFuture(future);
}

} // namespace Internal
} // namespace CppEditor
