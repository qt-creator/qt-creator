/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppfindreferences.h"
#include "cppmodelmanagerinterface.h"
#include "cpptoolsconstants.h"

#include <texteditor/basetexteditor.h>
#include <texteditor/basefilefind.h>
#include <find/searchresultwindow.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/filesearch.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <ASTVisitor.h>
#include <AST.h>
#include <Control.h>
#include <Literals.h>
#include <TranslationUnit.h>
#include <Symbols.h>
#include <Names.h>
#include <Scope.h>

#include <cplusplus/CppDocument.h>
#include <cplusplus/CppBindings.h>
#include <cplusplus/Overview.h>

#include <QtCore/QTime>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QtConcurrentMap>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <qtconcurrent/runextensions.h>

#include <functional>

using namespace CppTools::Internal;
using namespace CPlusPlus;

namespace {

class ProcessFile: public std::unary_function<QString, QList<Usage> >
{
    const CppTools::CppModelManagerInterface::WorkingCopy workingCopy;
    const Snapshot snapshot;
    Symbol *symbol;

public:
    ProcessFile(const CppTools::CppModelManagerInterface::WorkingCopy &workingCopy,
                const Snapshot snapshot,
                Symbol *symbol)
        : workingCopy(workingCopy), snapshot(snapshot), symbol(symbol)
    { }

    QList<Usage> operator()(const QString &fileName)
    {
        QList<Usage> usages;
        const Identifier *symbolId = symbol->identifier();

        if (Document::Ptr previousDoc = snapshot.document(fileName)) {
            Control *control = previousDoc->control();
            if (! control->findIdentifier(symbolId->chars(), symbolId->size()))
                return usages; // skip this document, it's not using symbolId.
        }

        QByteArray source;

        if (workingCopy.contains(fileName))
            source = snapshot.preprocessedCode(workingCopy.source(fileName), fileName);
        else {
            QFile file(fileName);
            if (! file.open(QFile::ReadOnly))
                return usages;

            const QString contents = QTextStream(&file).readAll(); // ### FIXME
            source = snapshot.preprocessedCode(contents, fileName);
        }

        Document::Ptr doc = snapshot.documentFromSource(source, fileName);
        doc->tokenize();

        Control *control = doc->control();
        if (control->findIdentifier(symbolId->chars(), symbolId->size()) != 0) {
            doc->check();

            FindUsages process(doc, snapshot);
            process.setGlobalNamespaceBinding(bind(doc, snapshot));

            process(symbol);
            usages = process.usages();
        }

        return usages;
    }
};

class UpdateUI: public std::binary_function<QList<Usage> &, QList<Usage>, void>
{
    QFutureInterface<Usage> *future;

public:
    UpdateUI(QFutureInterface<Usage> *future): future(future) {}

    void operator()(QList<Usage> &, const QList<Usage> &usages)
    {
        foreach (const Usage &u, usages)
            future->reportResult(u);

        future->setProgressValue(future->progressValue() + 1);
    }
};

} // end of anonymous namespace

CppFindReferences::CppFindReferences(CppTools::CppModelManagerInterface *modelManager)
    : QObject(modelManager),
      _modelManager(modelManager),
      _resultWindow(ExtensionSystem::PluginManager::instance()->getObject<Find::SearchResultWindow>())
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, SIGNAL(resultsReadyAt(int,int)), this, SLOT(displayResults(int,int)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
}

CppFindReferences::~CppFindReferences()
{
}

QList<int> CppFindReferences::references(Symbol *symbol,
                                         Document::Ptr doc,
                                         const Snapshot& snapshot) const
{
    QList<int> references;

    FindUsages findUsages(doc, snapshot);
    findUsages.setGlobalNamespaceBinding(bind(doc, snapshot));
    findUsages(symbol);
    references = findUsages.references();

    return references;
}

static void find_helper(QFutureInterface<Usage> &future,
                        const CppTools::CppModelManagerInterface::WorkingCopy workingCopy,
                        Snapshot snapshot,
                        Symbol *symbol)
{
    QTime tm;
    tm.start();

    const Identifier *symbolId = symbol->identifier();
    Q_ASSERT(symbolId != 0);

    const QString sourceFile = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
    QStringList files(sourceFile);

    if (symbol->isClass() || symbol->isForwardClassDeclaration()) {
        foreach (const Document::Ptr &doc, snapshot) {
            if (doc->fileName() == sourceFile)
                continue;

            Control *control = doc->control();

            if (control->findIdentifier(symbolId->chars(), symbolId->size()))
                files.append(doc->fileName());
        }
    } else {
        files += snapshot.filesDependingOn(sourceFile);
    }
    files.removeDuplicates();
    //qDebug() << "done in:" << tm.elapsed() << "number of files to parse:" << files.size();

    future.setProgressRange(0, files.size());

    ProcessFile process(workingCopy, snapshot, symbol);
    UpdateUI reduce(&future);

    QtConcurrent::blockingMappedReduced<QList<Usage> > (files, process, reduce);

    future.setProgressValue(files.size());
}

void CppFindReferences::findUsages(Symbol *symbol)
{
    Find::SearchResult *search = _resultWindow->startNewSearch(Find::SearchResultWindow::SearchOnly);

    connect(search, SIGNAL(activated(Find::SearchResultItem)),
            this, SLOT(openEditor(Find::SearchResultItem)));

    findAll_helper(symbol);
}

void CppFindReferences::renameUsages(Symbol *symbol)
{
    if (const Identifier *id = symbol->identifier()) {
        const QString textToReplace = QString::fromUtf8(id->chars(), id->size());

        Find::SearchResult *search = _resultWindow->startNewSearch(Find::SearchResultWindow::SearchAndReplace);
        _resultWindow->setTextToReplace(textToReplace);

        connect(search, SIGNAL(activated(Find::SearchResultItem)),
                this, SLOT(openEditor(Find::SearchResultItem)));

        connect(search, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>)),
                SLOT(onReplaceButtonClicked(QString,QList<Find::SearchResultItem>)));

        findAll_helper(symbol);
    }
}

void CppFindReferences::findAll_helper(Symbol *symbol)
{
    if (! (symbol && symbol->identifier()))
        return;

    _resultWindow->popup(true);

    const Snapshot snapshot = _modelManager->snapshot();
    const CppTools::CppModelManagerInterface::WorkingCopy workingCopy = _modelManager->workingCopy();

    Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();

    QFuture<Usage> result;

    result = QtConcurrent::run(&find_helper, workingCopy, snapshot, symbol);
    m_watcher.setFuture(result);

    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching..."),
                                                              CppTools::Constants::TASK_SEARCH);

    connect(progress, SIGNAL(clicked()), _resultWindow, SLOT(popup()));
}

void CppFindReferences::onReplaceButtonClicked(const QString &text,
                                               const QList<Find::SearchResultItem> &items)
{
    Core::EditorManager::instance()->hideEditorInfoBar(QLatin1String("CppEditor.Rename"));

    const QStringList fileNames = TextEditor::BaseFileFind::replaceAll(text, items);
    if (!fileNames.isEmpty()) {
        _modelManager->updateSourceFiles(fileNames);
        _resultWindow->hide();
    }
}

void CppFindReferences::displayResults(int first, int last)
{
    for (int index = first; index != last; ++index) {
        Usage result = m_watcher.future().resultAt(index);
        _resultWindow->addResult(result.path,
                                 result.line,
                                 result.lineText,
                                 result.col,
                                 result.len);
    }
}

void CppFindReferences::searchFinished()
{
    _resultWindow->finishSearch();
    emit changed();
}

void CppFindReferences::openEditor(const Find::SearchResultItem &item)
{
    TextEditor::BaseTextEditor::openEditorAt(item.fileName, item.lineNumber, item.searchTermStart);
}

