/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "filesystemfilter.h"
#include "locatorwidget.h"
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QDir>

using namespace Core;
using namespace Locator;
using namespace Locator::Internal;

FileSystemFilter::FileSystemFilter(EditorManager *editorManager, LocatorWidget *locatorWidget)
        : m_editorManager(editorManager), m_locatorWidget(locatorWidget), m_includeHidden(true)
{
    setId("Files in file system");
    setDisplayName(tr("Files in File System"));
    setShortcutString(QString(QLatin1Char('f')));
    setIncludedByDefault(false);
}

QList<FilterEntry> FileSystemFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QList<FilterEntry> value;
    QFileInfo entryInfo(entry);
    QString name = entryInfo.fileName();
    QString directory = entryInfo.path();
    QString filePath = entryInfo.filePath();
    if (entryInfo.isRelative()) {
        if (filePath.startsWith(QLatin1String("~/"))) {
            directory.replace(0, 1, QDir::homePath());
        } else {
            IEditor *editor = EditorManager::currentEditor();
            if (editor && !editor->document()->fileName().isEmpty()) {
                QFileInfo info(editor->document()->fileName());
                directory.prepend(info.absolutePath() + QLatin1Char('/'));
            }
        }
    }
    QDir dirInfo(directory);
    QDir::Filters dirFilter = QDir::Dirs|QDir::Drives|QDir::NoDot;
    QDir::Filters fileFilter = QDir::Files;
    if (m_includeHidden) {
        dirFilter |= QDir::Hidden;
        fileFilter |= QDir::Hidden;
    }
    QStringList dirs = dirInfo.entryList(dirFilter,
                                      QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
    QStringList files = dirInfo.entryList(fileFilter,
                                      QDir::Name|QDir::IgnoreCase|QDir::LocaleAware);
    foreach (const QString &dir, dirs) {
        if (future.isCanceled())
            break;
        if (name.isEmpty() || dir.startsWith(name, Qt::CaseInsensitive)) {
            const QString fullPath = dirInfo.filePath(dir);
            FilterEntry filterEntry(this, dir, QVariant());
            filterEntry.fileName = fullPath;
            value.append(filterEntry);
        }
    }
    // file names can match with +linenumber or :linenumber
    name = entry;
    const QString lineNoSuffix = EditorManager::splitLineNumber(&name);
    name = QFileInfo(name).fileName();
    foreach (const QString &file, files) {
        if (future.isCanceled())
            break;
        if (name.isEmpty() || file.startsWith(name, Qt::CaseInsensitive)) {
            const QString fullPath = dirInfo.filePath(file);
            FilterEntry filterEntry(this, file, QString(fullPath + lineNoSuffix));
            filterEntry.fileName = fullPath;
            value.append(filterEntry);
        }
    }
    return value;
}

void FileSystemFilter::accept(FilterEntry selection) const
{
    QString fileName = selection.fileName;
    QFileInfo info(fileName);
    if (info.isDir()) {
        QString value = shortcutString();
        value += QLatin1Char(' ');
        value += QDir::toNativeSeparators(info.absoluteFilePath() + QLatin1Char('/'));
        m_locatorWidget->show(value, value.length());
        return;
    }
    EditorManager::openEditor(selection.internalData.toString(), Id(),
                              EditorManager::ModeSwitch | EditorManager::CanContainLineNumber);
}

bool FileSystemFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    Ui::FileSystemFilterOptions ui;
    QDialog dialog(parent);
    ui.setupUi(&dialog);

    ui.hiddenFilesFlag->setChecked(m_includeHidden);
    ui.limitCheck->setChecked(!isIncludedByDefault());
    ui.shortcutEdit->setText(shortcutString());

    if (dialog.exec() == QDialog::Accepted) {
        m_includeHidden = ui.hiddenFilesFlag->isChecked();
        setShortcutString(ui.shortcutEdit->text().trimmed());
        setIncludedByDefault(!ui.limitCheck->isChecked());
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
