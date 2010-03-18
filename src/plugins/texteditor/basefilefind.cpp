/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "basefilefind.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <find/textfindconstants.h>
#include <find/searchresultwindow.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <utils/stylehelper.h>

#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>

using namespace Utils;
using namespace Find;
using namespace TextEditor;

BaseFileFind::BaseFileFind(SearchResultWindow *resultWindow)
  : m_resultWindow(resultWindow),
    m_isSearching(false),
    m_resultLabel(0),
    m_filterCombo(0),
    m_useRegExp(false),
    m_useRegExpCheckBox(0)
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(displayResult(int)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
}

bool BaseFileFind::isEnabled() const
{
    return !m_isSearching;
}

QStringList BaseFileFind::fileNameFilters() const
{
    QStringList filters;
    if (m_filterCombo && !m_filterCombo->currentText().isEmpty()) {
        const QStringList parts = m_filterCombo->currentText().split(QLatin1Char(','));
        foreach (const QString &part, parts) {
            const QString filter = part.trimmed();
            if (!filter.isEmpty()) {
                filters << filter;
            }
        }
    }
    return filters;
}

void BaseFileFind::findAll(const QString &txt, QTextDocument::FindFlags findFlags)
{
    m_isSearching = true;
    emit changed();
    if (m_filterCombo)
        updateComboEntries(m_filterCombo, false);
    m_watcher.setFuture(QFuture<FileSearchResult>());
    SearchResult *result = m_resultWindow->startNewSearch();
    connect(result, SIGNAL(activated(Find::SearchResultItem)), this, SLOT(openEditor(Find::SearchResultItem)));
    m_resultWindow->popup(true);
    if (m_useRegExp)
        m_watcher.setFuture(Utils::findInFilesRegExp(txt, files(), findFlags, ITextEditor::openedTextEditorsContents()));
    else
        m_watcher.setFuture(Utils::findInFiles(txt, files(), findFlags, ITextEditor::openedTextEditorsContents()));
    Core::FutureProgress *progress =
        Core::ICore::instance()->progressManager()->addTask(m_watcher.future(),
                                                                        "Search",
                                                                        Constants::TASK_SEARCH);
    progress->setWidget(createProgressWidget());
    connect(progress, SIGNAL(clicked()), m_resultWindow, SLOT(popup()));
}

void BaseFileFind::replaceAll(const QString &txt, QTextDocument::FindFlags findFlags)
{
    m_isSearching = true;
    emit changed();
    if (m_filterCombo)
        updateComboEntries(m_filterCombo, false);
    m_watcher.setFuture(QFuture<FileSearchResult>());
    SearchResult *result = m_resultWindow->startNewSearch(SearchResultWindow::SearchAndReplace);
    connect(result, SIGNAL(activated(Find::SearchResultItem)), this, SLOT(openEditor(Find::SearchResultItem)));
    connect(result, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>)),
            this, SLOT(doReplace(QString,QList<Find::SearchResultItem>)));
    m_resultWindow->popup(true);
    if (m_useRegExp)
        m_watcher.setFuture(Utils::findInFilesRegExp(txt, files(), findFlags, ITextEditor::openedTextEditorsContents()));
    else
        m_watcher.setFuture(Utils::findInFiles(txt, files(), findFlags, ITextEditor::openedTextEditorsContents()));
    Core::FutureProgress *progress =
        Core::ICore::instance()->progressManager()->addTask(m_watcher.future(),
                                                                        "Search",
                                                                        Constants::TASK_SEARCH);
    progress->setWidget(createProgressWidget());
    connect(progress, SIGNAL(clicked()), m_resultWindow, SLOT(popup()));
}

void BaseFileFind::doReplace(const QString &text,
                               const QList<Find::SearchResultItem> &items)
{
    QStringList files = replaceAll(text, items);
    Core::FileManager *fileManager = Core::ICore::instance()->fileManager();
    if (!files.isEmpty()) {
        fileManager->notifyFilesChangedInternally(files);
        m_resultWindow->hide();
    }
}

void BaseFileFind::displayResult(int index) {
    Utils::FileSearchResult result = m_watcher.future().resultAt(index);
    m_resultWindow->addResult(result.fileName,
                              result.lineNumber,
                              result.matchingLine,
                              result.matchStart,
                              result.matchLength);
    if (m_resultLabel)
        m_resultLabel->setText(tr("%1 found").arg(m_resultWindow->numberOfResults()));
}

void BaseFileFind::searchFinished()
{
    m_resultWindow->finishSearch();
    m_isSearching = false;
    m_resultLabel = 0;
    emit changed();
}

QWidget *BaseFileFind::createProgressWidget()
{
    m_resultLabel = new QLabel;
    m_resultLabel->setAlignment(Qt::AlignCenter);
    // ### TODO this setup should be done by style
    QFont f = m_resultLabel->font();
    f.setBold(true);
    f.setPointSizeF(StyleHelper::sidebarFontSize());
    m_resultLabel->setFont(f);
    m_resultLabel->setPalette(StyleHelper::sidebarFontPalette(m_resultLabel->palette()));
    m_resultLabel->setText(tr("%1 found").arg(m_resultWindow->numberOfResults()));
    return m_resultLabel;
}

QWidget *BaseFileFind::createPatternWidget()
{
/*
    QWidget *widget = new QWidget;
    QHBoxLayout *hlayout = new QHBoxLayout(widget);
    hlayout->setMargin(0);
    widget->setLayout(hlayout);
*/
    QString filterToolTip = tr("List of comma separated wildcard filters");
/*
    QLabel *label = new QLabel(tr("File &pattern:"));
    label->setToolTip(filterToolTip);
*/
/*
    hlayout->addWidget(label);
*/
    m_filterCombo = new QComboBox;
    m_filterCombo->setEditable(true);
    m_filterCombo->setModel(&m_filterStrings);
    m_filterCombo->setMaxCount(10);
    m_filterCombo->setMinimumContentsLength(10);
    m_filterCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_filterCombo->setInsertPolicy(QComboBox::InsertAtBottom);
    m_filterCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_filterCombo->setToolTip(filterToolTip);
    syncComboWithSettings(m_filterCombo, m_filterSetting);
/*
    label->setBuddy(m_filterCombo);
    hlayout->addWidget(m_filterCombo);
*/
    return m_filterCombo;
}

QWidget *BaseFileFind::createRegExpWidget()
{
    m_useRegExpCheckBox = new QCheckBox(tr("Use Regular E&xpressions"));
    m_useRegExpCheckBox->setChecked(m_useRegExp);
    connect(m_useRegExpCheckBox, SIGNAL(toggled(bool)), this, SLOT(syncRegExpSetting(bool)));
    return m_useRegExpCheckBox;
}

void BaseFileFind::writeCommonSettings(QSettings *settings)
{
    settings->setValue("filters", m_filterStrings.stringList());
    if (m_filterCombo)
        settings->setValue("currentFilter", m_filterCombo->currentText());
    settings->setValue("useRegExp", m_useRegExp);
}

void BaseFileFind::readCommonSettings(QSettings *settings, const QString &defaultFilter)
{
    QStringList filters = settings->value("filters").toStringList();
    m_filterSetting = settings->value("currentFilter").toString();
    m_useRegExp = settings->value("useRegExp", false).toBool();
    if (m_useRegExpCheckBox)
        m_useRegExpCheckBox->setChecked(m_useRegExp);
    if (filters.isEmpty())
        filters << defaultFilter;
    if (m_filterSetting.isEmpty())
        m_filterSetting = filters.first();
    m_filterStrings.setStringList(filters);
    if (m_filterCombo)
        syncComboWithSettings(m_filterCombo, m_filterSetting);
}

void BaseFileFind::syncComboWithSettings(QComboBox *combo, const QString &setting)
{
    if (!combo)
        return;
    int index = combo->findText(setting);
    if (index < 0)
        combo->setEditText(setting);
    else
        combo->setCurrentIndex(index);
}

void BaseFileFind::updateComboEntries(QComboBox *combo, bool onTop)
{
    int index = combo->findText(combo->currentText());
    if (index < 0) {
        if (onTop) {
            combo->insertItem(0, combo->currentText());
        } else {
            combo->addItem(combo->currentText());
        }
        combo->setCurrentIndex(combo->findText(combo->currentText()));
    }
}

void BaseFileFind::syncRegExpSetting(bool useRegExp)
{
    m_useRegExp = useRegExp;
}

void BaseFileFind::openEditor(const Find::SearchResultItem &item)
{
    TextEditor::BaseTextEditor::openEditorAt(item.fileName, item.lineNumber, item.searchTermStart);
}

// #pragma mark Static methods

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

QStringList BaseFileFind::replaceAll(const QString &text,
                               const QList<Find::SearchResultItem> &items)
{
    if (text.isEmpty() || items.isEmpty())
        return QStringList();

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

    return changes.keys();
}
