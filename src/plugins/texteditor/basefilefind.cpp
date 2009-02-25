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

#include "basefilefind.h"

#include <coreplugin/icore.h>
#include <coreplugin/stylehelper.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <find/textfindconstants.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>

using namespace Core::Utils;
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
        QStringList parts = m_filterCombo->currentText().split(",");
        foreach (const QString &part, parts) {
            QString filter = part.trimmed();
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
    updateComboEntries(m_filterCombo, false);
    m_watcher.setFuture(QFuture<FileSearchResult>());
    m_resultWindow->clearContents();
    m_resultWindow->popup(true);
    if (m_useRegExp)
        m_watcher.setFuture(Core::Utils::findInFilesRegExp(txt, files(), findFlags));
    else
        m_watcher.setFuture(Core::Utils::findInFiles(txt, files(), findFlags));
    Core::FutureProgress *progress = 
        Core::ICore::instance()->progressManager()->addTask(m_watcher.future(),
                                                                        "Search",
                                                                        Constants::TASK_SEARCH);
    progress->setWidget(createProgressWidget());
    connect(progress, SIGNAL(clicked()), m_resultWindow, SLOT(popup()));
}

void BaseFileFind::displayResult(int index) {
    Core::Utils::FileSearchResult result = m_watcher.future().resultAt(index);
    ResultWindowItem *item = m_resultWindow->addResult(result.fileName,
                              result.lineNumber,
                              result.matchingLine,
                              result.matchStart,
                              result.matchLength);
    if (item)
        connect(item, SIGNAL(activated(const QString&,int,int)), this, SLOT(openEditor(const QString&,int,int)));

    if (m_resultLabel)
        m_resultLabel->setText(tr("%1 found").arg(m_resultWindow->numberOfResults()));
}

void BaseFileFind::searchFinished()
{
    m_isSearching = false;
    m_resultLabel = 0;
    emit changed();
}

QWidget *BaseFileFind::createProgressWidget()
{
    m_resultLabel = new QLabel;
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

void BaseFileFind::openEditor(const QString &fileName, int line, int column)
{
    TextEditor::BaseTextEditor::openEditorAt(fileName, line, column);
}
