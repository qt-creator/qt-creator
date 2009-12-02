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
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <qtconcurrent/runextensions.h>

using namespace CppTools::Internal;
using namespace CPlusPlus;

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
    const Identifier *id = 0;
    if (const Identifier *symbolId = symbol->identifier())
        id = doc->control()->findIdentifier(symbolId->chars(), symbolId->size());

    QList<int> references;

    if (! id)
        return references;

    TranslationUnit *translationUnit = doc->translationUnit();
    Q_ASSERT(translationUnit != 0);

    FindUsages findUsages(doc, snapshot, /*future = */ 0);
    findUsages.setGlobalNamespaceBinding(bind(doc, snapshot));
    findUsages(symbol, id, translationUnit->ast());
    references = findUsages.references();

    return references;
}

static void find_helper(QFutureInterface<Usage> &future,
                        const QMap<QString, QString> wl,
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

    for (int i = 0; i < files.size(); ++i) {
        if (future.isPaused())
            future.waitForResume();

        if (future.isCanceled())
            break;

        const QString &fileName = files.at(i);
        future.setProgressValueAndText(i, QFileInfo(fileName).fileName());

        if (Document::Ptr previousDoc = snapshot.value(fileName)) {
            Control *control = previousDoc->control();
            const Identifier *id = control->findIdentifier(symbolId->chars(), symbolId->size());
            if (! id)
                continue; // skip this document, it's not using symbolId.
        }

        QByteArray source;

        if (wl.contains(fileName))
            source = snapshot.preprocessedCode(wl.value(fileName), fileName);
        else {
            QFile file(fileName);
            if (! file.open(QFile::ReadOnly))
                continue;

            const QString contents = QTextStream(&file).readAll(); // ### FIXME
            source = snapshot.preprocessedCode(contents, fileName);
        }

        Document::Ptr doc = snapshot.documentFromSource(source, fileName);
        doc->tokenize();

        Control *control = doc->control();
        if (const Identifier *id = control->findIdentifier(symbolId->chars(), symbolId->size())) {
            QTime tm;
            tm.start();
            doc->parse();

            //qDebug() << "***" << unit->fileName() << "parsed in:" << tm.elapsed();

            tm.start();
            doc->check();
            //qDebug() << "***" << unit->fileName() << "checked in:" << tm.elapsed();

            tm.start();

            FindUsages process(doc, snapshot, &future);
            process.setGlobalNamespaceBinding(bind(doc, snapshot));

            TranslationUnit *unit = doc->translationUnit();
            process(symbol, id, unit->ast());

            //qDebug() << "***" << unit->fileName() << "processed in:" << tm.elapsed();
        }
    }

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
    const QMap<QString, QString> wl = _modelManager->workingCopy();

    Core::ProgressManager *progressManager = Core::ICore::instance()->progressManager();

    QFuture<Usage> result;

    result = QtConcurrent::run(&find_helper, wl, snapshot, symbol);
    m_watcher.setFuture(result);

    Core::FutureProgress *progress = progressManager->addTask(result, tr("Searching..."),
                                                              CppTools::Constants::TASK_SEARCH);

    connect(progress, SIGNAL(clicked()), _resultWindow, SLOT(popup()));
}

static void applyChanges(QTextDocument *doc, const QString &text, const QList<Find::SearchResultItem> &items)
{
    QList<QTextCursor> cursors;

    foreach (const Find::SearchResultItem &item, items) {
        const int blockNumber = item.lineNumber - 1;
        QTextCursor tc(doc->findBlockByNumber(blockNumber));

        const int cursorPosition = tc.position() + item.searchTermStart;

        int cursorIndex = 0;
        for (; cursorIndex < cursors.size(); ++cursorIndex) {
            const QTextCursor &tc = cursors.at(cursorIndex);

            if (tc.position() == cursorPosition)
                break;
        }

        if (cursorIndex != cursors.size())
            continue; // skip this change.

        tc.setPosition(cursorPosition);
        tc.setPosition(tc.position() + item.searchTermLength,
                       QTextCursor::KeepAnchor);
        cursors.append(tc);
    }

    foreach (QTextCursor tc, cursors)
        tc.insertText(text);
}

void CppFindReferences::onReplaceButtonClicked(const QString &text,
                                               const QList<Find::SearchResultItem> &items)
{
    Core::EditorManager::instance()->hideEditorInfoBar(QLatin1String("CppEditor.Rename"));

    if (text.isEmpty())
        return;

    QHash<QString, QList<Find::SearchResultItem> > changes;

    foreach (const Find::SearchResultItem &item, items)
        changes[item.fileName].append(item);

    Core::EditorManager *editorManager = Core::EditorManager::instance();

    QHashIterator<QString, QList<Find::SearchResultItem> > it(changes);
    while (it.hasNext()) {
        it.next();

        const QString fileName = it.key();
        const QList<Find::SearchResultItem> items = it.value();

        const QList<Core::IEditor *> editors = editorManager->editorsForFileName(fileName);
        TextEditor::BaseTextEditor *textEditor = 0;
        foreach (Core::IEditor *editor, editors) {
            textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor->widget());
            if (textEditor != 0)
                break;
        }

        if (textEditor != 0) {
            QTextCursor tc = textEditor->textCursor();
            tc.beginEditBlock();
            applyChanges(textEditor->document(), text, items);
            tc.endEditBlock();
        } else {
            QFile file(fileName);

            if (file.open(QFile::ReadOnly)) {
                QTextStream stream(&file);
                // ### set the encoding
                const QString plainText = stream.readAll();
                file.close();

                QTextDocument doc;
                doc.setPlainText(plainText);

                applyChanges(&doc, text, items);

                QFile newFile(fileName);
                if (newFile.open(QFile::WriteOnly)) {
                    QTextStream stream(&newFile);
                    // ### set the encoding
                    stream << doc.toPlainText();
                }
            }
        }
    }

    const QStringList fileNames = changes.keys();
    _modelManager->updateSourceFiles(fileNames);
    _resultWindow->hide();
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

