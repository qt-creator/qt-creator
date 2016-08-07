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

#include "filesystemfilter.h"
#include "locatorwidget.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <utils/fileutils.h>

#include <QDir>

using namespace Core;
using namespace Core::Internal;

namespace {

QList<LocatorFilterEntry> *categorize(const QString &entry, const QString &candidate,
                               Qt::CaseSensitivity caseSensitivity,
                               QList<LocatorFilterEntry> *betterEntries, QList<LocatorFilterEntry> *goodEntries)
{
    if (entry.isEmpty() || candidate.startsWith(entry, caseSensitivity))
        return betterEntries;
    else if (candidate.contains(entry, caseSensitivity))
        return goodEntries;
    return 0;
}

} // anynoumous namespace

FileSystemFilter::FileSystemFilter(LocatorWidget *locatorWidget)
        : m_locatorWidget(locatorWidget)
{
    setId("Files in file system");
    setDisplayName(tr("Files in File System"));
    setShortcutString(QString(QLatin1Char('f')));
    setIncludedByDefault(false);
}

void FileSystemFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_currentDocumentDirectory = DocumentManager::fileDialogInitialDirectory();
}

QList<LocatorFilterEntry> FileSystemFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                       const QString &entry)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    QFileInfo entryInfo(entry);
    const QString entryFileName = entryInfo.fileName();
    QString directory = entryInfo.path();
    QString filePath = entryInfo.filePath();
    if (entryInfo.isRelative()) {
        if (filePath.startsWith(QLatin1String("~/")))
            directory.replace(0, 1, QDir::homePath());
        else if (!m_currentDocumentDirectory.isEmpty())
            directory.prepend(m_currentDocumentDirectory + "/");
    }
    QDir dirInfo(directory);
    QDir::Filters dirFilter = QDir::Dirs|QDir::Drives|QDir::NoDot;
    QDir::Filters fileFilter = QDir::Files;
    if (m_includeHidden) {
        dirFilter |= QDir::Hidden;
        fileFilter |= QDir::Hidden;
    }
    // use only 'name' for case sensitivity decision, because we need to make the path
    // match the case on the file system for case-sensitive file systems
    const Qt::CaseSensitivity caseSensitivity_ = caseSensitivity(entryFileName);
    QStringList dirs = dirInfo.entryList(dirFilter,
                                      QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
    QStringList files = dirInfo.entryList(fileFilter,
                                      QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
    foreach (const QString &dir, dirs) {
        if (future.isCanceled())
            break;
        if (QList<LocatorFilterEntry> *category = categorize(entryFileName, dir, caseSensitivity_, &betterEntries,
                                                      &goodEntries)) {
            const QString fullPath = dirInfo.filePath(dir);
            LocatorFilterEntry filterEntry(this, dir, QVariant());
            filterEntry.fileName = fullPath;
            category->append(filterEntry);
        }
    }
    // file names can match with +linenumber or :linenumber
    const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(entry);
    const QString fileName = QFileInfo(fp.filePath).fileName();
    foreach (const QString &file, files) {
        if (future.isCanceled())
            break;
        if (QList<LocatorFilterEntry> *category = categorize(fileName, file, caseSensitivity_, &betterEntries,
                                                      &goodEntries)) {
            const QString fullPath = dirInfo.filePath(file);
            LocatorFilterEntry filterEntry(this, file, QString(fullPath + fp.postfix));
            filterEntry.fileName = fullPath;
            category->append(filterEntry);
        }
    }
    betterEntries.append(goodEntries);

    // "create and open" functionality
    const QString fullFilePath = dirInfo.filePath(fileName);
    if (!QFileInfo::exists(fullFilePath) && dirInfo.exists()) {
        LocatorFilterEntry createAndOpen(this, tr("Create and Open \"%1\"").arg(entry), fullFilePath);
        createAndOpen.extraInfo = Utils::FileUtils::shortNativePath(
                    Utils::FileName::fromString(dirInfo.absolutePath()));
        betterEntries.append(createAndOpen);
    }

    return betterEntries;
}

void FileSystemFilter::accept(LocatorFilterEntry selection) const
{
    QString fileName = selection.fileName;
    QFileInfo info(fileName);
    if (info.isDir()) {
        QString value = shortcutString();
        value += QLatin1Char(' ');
        value += QDir::toNativeSeparators(info.absoluteFilePath() + QLatin1Char('/'));
        m_locatorWidget->show(value, value.length());
        return;
    } else if (!info.exists()) {
        QFile file(selection.internalData.toString());
        file.open(QFile::WriteOnly);
        file.close();
    }
    const QFileInfo fileInfo(selection.internalData.toString());
    const QString cleanedFilePath = QDir::cleanPath(fileInfo.absoluteFilePath());
    EditorManager::openEditor(cleanedFilePath, Id(), EditorManager::CanContainLineAndColumnNumber);
}

bool FileSystemFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    Ui::FileSystemFilterOptions ui;
    QDialog dialog(parent);
    ui.setupUi(&dialog);
    dialog.setWindowTitle(ILocatorFilter::msgConfigureDialogTitle());
    ui.prefixLabel->setText(ILocatorFilter::msgPrefixLabel());
    ui.prefixLabel->setToolTip(ILocatorFilter::msgPrefixToolTip());
    ui.includeByDefault->setText(msgIncludeByDefault());
    ui.includeByDefault->setToolTip(msgIncludeByDefaultToolTip());
    ui.hiddenFilesFlag->setChecked(m_includeHidden);
    ui.includeByDefault->setChecked(isIncludedByDefault());
    ui.shortcutEdit->setText(shortcutString());

    if (dialog.exec() == QDialog::Accepted) {
        m_includeHidden = ui.hiddenFilesFlag->isChecked();
        setShortcutString(ui.shortcutEdit->text().trimmed());
        setIncludedByDefault(ui.includeByDefault->isChecked());
        return true;
    }
    return false;
}

QByteArray FileSystemFilter::saveState() const
{
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << m_includeHidden;
    out << shortcutString();
    out << isIncludedByDefault();
    return value;
}

bool FileSystemFilter::restoreState(const QByteArray &state)
{
    QDataStream in(state);
    in >> m_includeHidden;

    // An attempt to prevent setting this on old configuration
    if (!in.atEnd()) {
        QString shortcut;
        bool defaultFilter;
        in >> shortcut;
        in >> defaultFilter;
        setShortcutString(shortcut);
        setIncludedByDefault(defaultFilter);
    }

    return true;
}
