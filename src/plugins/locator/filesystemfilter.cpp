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

#include "filesystemfilter.h"
#include "locatorwidget.h"
#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QDir>

using namespace Core;
using namespace Locator;
using namespace Locator::Internal;

FileSystemFilter::FileSystemFilter(EditorManager *editorManager, LocatorWidget *locatorWidget)
        : m_editorManager(editorManager), m_locatorWidget(locatorWidget), m_includeHidden(true)
{
    setShortcutString(QString(QLatin1Char('f')));
    setIncludedByDefault(false);
}

QList<FilterEntry> FileSystemFilter::matchesFor(const QString &entry)
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
            IEditor *editor = m_editorManager->currentEditor();
            if (editor && !editor->file()->fileName().isEmpty()) {
                QFileInfo info(editor->file()->fileName());
                directory.prepend(info.absolutePath() + QLatin1Char('/'));
            }
        }
    }
    QDir dirInfo(directory);
    QDir::Filters dirFilter = QDir::Dirs|QDir::Drives;
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
        if (dir != QLatin1String(".") && (name.isEmpty() || dir.startsWith(name, Qt::CaseInsensitive))) {
            FilterEntry entry(this, dir, dirInfo.filePath(dir));
            entry.resolveFileIcon = true;
            value.append(entry);
        }
    }
    foreach (const QString &file, files) {
        if (name.isEmpty() || file.startsWith(name, Qt::CaseInsensitive)) {
            const QString fullPath = dirInfo.filePath(file);
            FilterEntry entry(this, file, fullPath);
            entry.resolveFileIcon = true;
            value.append(entry);
        }
    }
    return value;
}

void FileSystemFilter::accept(FilterEntry selection) const
{
    QFileInfo info(selection.internalData.toString());
    if (info.isDir()) {
        QString value = shortcutString();
        value += QLatin1Char(' ');
        value += QDir::toNativeSeparators(info.absoluteFilePath() + QLatin1Char('/'));
        m_locatorWidget->show(value, value.length());
        return;
    }
    m_editorManager->openEditor(selection.internalData.toString());
    m_editorManager->ensureEditorManagerVisible();
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
