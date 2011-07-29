/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
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
#include <utils/fileutils.h>

#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtGui/QFileDialog>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QTextBlock>

using namespace Utils;
using namespace Find;
using namespace TextEditor;

BaseFileFind::BaseFileFind(SearchResultWindow *resultWindow)
  : m_resultWindow(resultWindow),
    m_isSearching(false),
    m_resultLabel(0),
    m_filterCombo(0)
{
    m_watcher.setPendingResultsLimit(1);
    connect(&m_watcher, SIGNAL(resultReadyAt(int)), this, SLOT(displayResult(int)));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(searchFinished()));
}

bool BaseFileFind::isEnabled() const
{
    return !m_isSearching;
}

bool BaseFileFind::canCancel() const
{
    return m_isSearching;
}

void BaseFileFind::cancel()
{
    m_watcher.cancel();
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

void BaseFileFind::findAll(const QString &txt, Find::FindFlags findFlags)
{
    m_isSearching = true;
    emit changed();
    if (m_filterCombo)
        updateComboEntries(m_filterCombo, true);
    m_watcher.setFuture(QFuture<FileSearchResultList>());
    SearchResult *result = m_resultWindow->startNewSearch();
    connect(result, SIGNAL(activated(Find::SearchResultItem)), this, SLOT(openEditor(Find::SearchResultItem)));
    m_resultWindow->popup(true);
    if (findFlags & Find::FindRegularExpression) {
        m_watcher.setFuture(Utils::findInFilesRegExp(txt, files(),
            textDocumentFlagsForFindFlags(findFlags), ITextEditor::openedTextEditorsContents()));
    } else {
        m_watcher.setFuture(Utils::findInFiles(txt, files(),
            textDocumentFlagsForFindFlags(findFlags), ITextEditor::openedTextEditorsContents()));
    }
    Core::FutureProgress *progress =
        Core::ICore::instance()->progressManager()->addTask(m_watcher.future(),
                                                                        tr("Search"),
                                                                        Constants::TASK_SEARCH);
    progress->setWidget(createProgressWidget());
    connect(progress, SIGNAL(clicked()), m_resultWindow, SLOT(popup()));
}

void BaseFileFind::replaceAll(const QString &txt, Find::FindFlags findFlags)
{
    m_isSearching = true;
    emit changed();
    if (m_filterCombo)
        updateComboEntries(m_filterCombo, true);
    m_watcher.setFuture(QFuture<FileSearchResultList>());
    SearchResult *result = m_resultWindow->startNewSearch(
            SearchResultWindow::SearchAndReplace, QLatin1String("TextEditor"));
    connect(result, SIGNAL(activated(Find::SearchResultItem)), this, SLOT(openEditor(Find::SearchResultItem)));
    connect(result, SIGNAL(replaceButtonClicked(QString,QList<Find::SearchResultItem>)),
            this, SLOT(doReplace(QString,QList<Find::SearchResultItem>)));
    m_resultWindow->popup(true);
    if (findFlags & Find::FindRegularExpression) {
        m_watcher.setFuture(Utils::findInFilesRegExp(txt, files(),
            textDocumentFlagsForFindFlags(findFlags), ITextEditor::openedTextEditorsContents()));
    } else {
        m_watcher.setFuture(Utils::findInFiles(txt, files(),
            textDocumentFlagsForFindFlags(findFlags), ITextEditor::openedTextEditorsContents()));
    }
    Core::FutureProgress *progress =
        Core::ICore::instance()->progressManager()->addTask(m_watcher.future(),
                                                                        tr("Search"),
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
    Utils::FileSearchResultList results = m_watcher.future().resultAt(index);
    QList<Find::SearchResultItem> items; // this conversion is stupid...
    foreach (const Utils::FileSearchResult &result, results) {
        Find::SearchResultItem item;
        item.path = QStringList() << QDir::toNativeSeparators(result.fileName);
        item.lineNumber = result.lineNumber;
        item.text = result.matchingLine;
        item.textMarkLength = result.matchLength;
        item.textMarkPos = result.matchStart;
        item.useTextEditorFont = true;
        item.userData = result.regexpCapturedTexts;
        items << item;
    }
    m_resultWindow->addResults(items, Find::SearchResultWindow::AddOrdered);
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
    QString filterToolTip = tr("List of comma separated wildcard filters");
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
    return m_filterCombo;
}

void BaseFileFind::writeCommonSettings(QSettings *settings)
{
    settings->setValue("filters", m_filterStrings.stringList());
    if (m_filterCombo)
        settings->setValue("currentFilter", m_filterCombo->currentText());
}

void BaseFileFind::readCommonSettings(QSettings *settings, const QString &defaultFilter)
{
    QStringList filters = settings->value("filters").toStringList();
    m_filterSetting = settings->value("currentFilter").toString();
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

void BaseFileFind::openEditor(const Find::SearchResultItem &item)
{
    if (item.path.size() > 0) {
        TextEditor::BaseTextEditorWidget::openEditorAt(QDir::fromNativeSeparators(item.path.first()), item.lineNumber, item.textMarkPos,
                                                 QString(), Core::EditorManager::ModeSwitch);
    } else {
        Core::EditorManager::instance()->openEditor(item.text, QString(), Core::EditorManager::ModeSwitch);
    }
}

// #pragma mark Static methods

static void applyChanges(QTextDocument *doc, const QString &text, const QList<Find::SearchResultItem> &items)
{
    QList<QPair<QTextCursor, QString> > changes;

    foreach (const Find::SearchResultItem &item, items) {
        const int blockNumber = item.lineNumber - 1;
        QTextCursor tc(doc->findBlockByNumber(blockNumber));

        const int cursorPosition = tc.position() + item.textMarkPos;

        int cursorIndex = 0;
        for (; cursorIndex < changes.size(); ++cursorIndex) {
            const QTextCursor &otherTc = changes.at(cursorIndex).first;

            if (otherTc.position() == cursorPosition)
                break;
        }

        if (cursorIndex != changes.size())
            continue; // skip this change.

        tc.setPosition(cursorPosition);
        tc.setPosition(tc.position() + item.textMarkLength,
                       QTextCursor::KeepAnchor);
        QString substitutionText;
        if (item.userData.canConvert<QStringList>() && !item.userData.toStringList().isEmpty())
            substitutionText = Utils::expandRegExpReplacement(text, item.userData.toStringList());
        else
            substitutionText = text;
        changes.append(QPair<QTextCursor, QString>(tc, substitutionText));
    }

    for (int i = 0; i < changes.size(); ++i) {
        QPair<QTextCursor, QString> &cursor = changes[i];
        cursor.first.insertText(cursor.second);
    }
}

QStringList BaseFileFind::replaceAll(const QString &text,
                               const QList<Find::SearchResultItem> &items)
{
    if (items.isEmpty())
        return QStringList();

    QHash<QString, QList<Find::SearchResultItem> > changes;

    foreach (const Find::SearchResultItem &item, items)
        changes[QDir::fromNativeSeparators(item.path.first())].append(item);

    Core::EditorManager *editorManager = Core::EditorManager::instance();

    QHashIterator<QString, QList<Find::SearchResultItem> > it(changes);
    while (it.hasNext()) {
        it.next();

        const QString fileName = it.key();
        const QList<Find::SearchResultItem> changeItems = it.value();

        const QList<Core::IEditor *> editors = editorManager->editorsForFileName(fileName);
        TextEditor::BaseTextEditorWidget *textEditor = 0;
        foreach (Core::IEditor *editor, editors) {
            textEditor = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
            if (textEditor != 0)
                break;
        }

        if (textEditor != 0) {
            QTextCursor tc = textEditor->textCursor();
            tc.beginEditBlock();
            applyChanges(textEditor->document(), text, changeItems);
            tc.endEditBlock();
        } else {
            Utils::FileReader reader;
            if (reader.fetch(fileName, Core::ICore::instance()->mainWindow())) {
                // Keep track of line ending since QTextDocument is '\n' based.
                bool convertLineEnding = false;
                const QByteArray &data = reader.data();
                const int lf = data.indexOf('\n');
                if (lf > 0 && data.at(lf - 1) == '\r')
                    convertLineEnding = true;

                QTextDocument doc;
                // ### set the encoding
                doc.setPlainText(QString::fromLocal8Bit(data));
                applyChanges(&doc, text, changeItems);
                QString plainText = doc.toPlainText();

                if (convertLineEnding)
                    plainText.replace(QLatin1Char('\n'), QLatin1String("\r\n"));

                Utils::FileSaver saver(fileName);
                if (!saver.hasError()) {
                    QTextStream stream(saver.file());
                    // ### set the encoding
                    stream << plainText;
                    saver.setResult(&stream);
                }
                saver.finalize(Core::ICore::instance()->mainWindow());
            }
        }
    }

    return changes.keys();
}
