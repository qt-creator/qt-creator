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

#include "directoryfilter.h"
#include "ui_directoryfilter.h"

#include "locator.h"

#include <coreplugin/coreconstants.h>
#include <utils/algorithm.h>
#include <utils/filesearch.h>

#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>

using namespace Utils;

namespace Core {

/*!
    \class Core::DirectoryFilter
    \inmodule QtCreator
    \internal
*/

const char kDisplayNameKey[] = "displayName";
const char kDirectoriesKey[] = "directories";
const char kFiltersKey[] = "filters";
const char kFilesKey[] = "files";
const char kExclusionFiltersKey[] = "exclusionFilters";

const QStringList kFiltersDefault = {"*.h", "*.cpp", "*.ui", "*.qrc"};
const QStringList kExclusionFiltersDefault = {"*/.git/*", "*/.cvs/*", "*/.svn/*"};

static QString defaultDisplayName()
{
    return DirectoryFilter::tr("Generic Directory Filter");
}

DirectoryFilter::DirectoryFilter(Id id)
    : m_filters(kFiltersDefault)
    , m_exclusionFilters(kExclusionFiltersDefault)
{
    setId(id);
    setDefaultIncludedByDefault(true);
    setDisplayName(defaultDisplayName());
}

void DirectoryFilter::saveState(QJsonObject &object) const
{
    QMutexLocker locker(&m_lock); // m_files is modified in other thread

    if (displayName() != defaultDisplayName())
        object.insert(kDisplayNameKey, displayName());
    if (!m_directories.isEmpty())
        object.insert(kDirectoriesKey, QJsonArray::fromStringList(m_directories));
    if (m_filters != kFiltersDefault)
        object.insert(kFiltersKey, QJsonArray::fromStringList(m_filters));
    if (!m_files.isEmpty())
        object.insert(kFilesKey,
                      QJsonArray::fromStringList(
                          Utils::transform(m_files, &Utils::FilePath::toString)));
    if (m_exclusionFilters != kExclusionFiltersDefault)
        object.insert(kExclusionFiltersKey, QJsonArray::fromStringList(m_exclusionFilters));
}

static QStringList toStringList(const QJsonArray &array)
{
    return Utils::transform(array.toVariantList(), &QVariant::toString);
}

void DirectoryFilter::restoreState(const QJsonObject &object)
{
    QMutexLocker locker(&m_lock);
    setDisplayName(object.value(kDisplayNameKey).toString(defaultDisplayName()));
    m_directories = toStringList(object.value(kDirectoriesKey).toArray());
    m_filters = toStringList(
        object.value(kFiltersKey).toArray(QJsonArray::fromStringList(kFiltersDefault)));
    m_files = Utils::transform(toStringList(object.value(kFilesKey).toArray()),
                               &FilePath::fromString);
    m_exclusionFilters = toStringList(
        object.value(kExclusionFiltersKey)
            .toArray(QJsonArray::fromStringList(kExclusionFiltersDefault)));
}

void DirectoryFilter::restoreState(const QByteArray &state)
{
    if (isOldSetting(state)) {
        // TODO read old settings, remove some time after Qt Creator 4.15
        QMutexLocker locker(&m_lock);

        QString name;
        QStringList directories;
        QString shortcut;
        bool defaultFilter;
        QStringList files;

        QDataStream in(state);
        in >> name;
        in >> directories;
        in >> m_filters;
        in >> shortcut;
        in >> defaultFilter;
        in >> files;
        m_files = Utils::transform(files, &Utils::FilePath::fromString);
        if (!in.atEnd()) // Qt Creator 4.3 and later
            in >> m_exclusionFilters;
        else
            m_exclusionFilters.clear();

        if (m_isCustomFilter)
            m_directories = directories;
        setDisplayName(name);
        setShortcutString(shortcut);
        setIncludedByDefault(defaultFilter);

        locker.unlock();
        updateFileIterator();
    } else {
        ILocatorFilter::restoreState(state);
    }
}

bool DirectoryFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    if (!m_ui) {
        m_ui = new Internal::Ui::DirectoryFilterOptions;
    }
    bool success = false;
    QDialog dialog(parent);
    m_dialog = &dialog;
    m_ui->setupUi(&dialog);
    dialog.setWindowTitle(ILocatorFilter::msgConfigureDialogTitle());
    m_ui->prefixLabel->setText(ILocatorFilter::msgPrefixLabel());
    m_ui->prefixLabel->setToolTip(ILocatorFilter::msgPrefixToolTip());
    m_ui->defaultFlag->setText(ILocatorFilter::msgIncludeByDefault());
    m_ui->defaultFlag->setToolTip(ILocatorFilter::msgIncludeByDefaultToolTip());
    m_ui->nameEdit->setText(displayName());
    m_ui->nameEdit->selectAll();
    connect(m_ui->addButton,
            &QPushButton::clicked,
            this,
            &DirectoryFilter::handleAddDirectory,
            Qt::DirectConnection);
    connect(m_ui->editButton,
            &QPushButton::clicked,
            this,
            &DirectoryFilter::handleEditDirectory,
            Qt::DirectConnection);
    connect(m_ui->removeButton,
            &QPushButton::clicked,
            this,
            &DirectoryFilter::handleRemoveDirectory,
            Qt::DirectConnection);
    connect(m_ui->directoryList,
            &QListWidget::itemSelectionChanged,
            this,
            &DirectoryFilter::updateOptionButtons,
            Qt::DirectConnection);
    m_ui->directoryList->clear();
    // Note: assuming we only change m_directories in the Gui thread,
    // we don't need to protect it here with mutex
    m_ui->directoryList->addItems(m_directories);
    m_ui->nameLabel->setVisible(m_isCustomFilter);
    m_ui->nameEdit->setVisible(m_isCustomFilter);
    m_ui->directoryLabel->setVisible(m_isCustomFilter);
    m_ui->directoryList->setVisible(m_isCustomFilter);
    m_ui->addButton->setVisible(m_isCustomFilter);
    m_ui->editButton->setVisible(m_isCustomFilter);
    m_ui->removeButton->setVisible(m_isCustomFilter);
    m_ui->filePatternLabel->setText(Utils::msgFilePatternLabel());
    m_ui->filePatternLabel->setBuddy(m_ui->filePattern);
    m_ui->filePattern->setToolTip(Utils::msgFilePatternToolTip());
    // Note: assuming we only change m_filters in the Gui thread,
    // we don't need to protect it here with mutex
    m_ui->filePattern->setText(Utils::transform(m_filters, &QDir::toNativeSeparators).join(','));
    m_ui->exclusionPatternLabel->setText(Utils::msgExclusionPatternLabel());
    m_ui->exclusionPatternLabel->setBuddy(m_ui->exclusionPattern);
    m_ui->exclusionPattern->setToolTip(Utils::msgFilePatternToolTip());
    // Note: assuming we only change m_exclusionFilters in the Gui thread,
    // we don't need to protect it here with mutex
    m_ui->exclusionPattern->setText(
        Utils::transform(m_exclusionFilters, &QDir::toNativeSeparators).join(','));
    m_ui->shortcutEdit->setText(shortcutString());
    m_ui->defaultFlag->setChecked(isIncludedByDefault());
    updateOptionButtons();
    dialog.adjustSize();
    if (dialog.exec() == QDialog::Accepted) {
        QMutexLocker locker(&m_lock);
        bool directoriesChanged = false;
        const QStringList oldDirectories = m_directories;
        const QStringList oldFilters = m_filters;
        const QStringList oldExclusionFilters = m_exclusionFilters;
        setDisplayName(m_ui->nameEdit->text().trimmed());
        m_directories.clear();
        const int oldCount = oldDirectories.count();
        const int newCount = m_ui->directoryList->count();
        if (oldCount != newCount)
            directoriesChanged = true;
        for (int i = 0; i < newCount; ++i) {
            m_directories.append(m_ui->directoryList->item(i)->text());
            if (!directoriesChanged && m_directories.at(i) != oldDirectories.at(i))
                directoriesChanged = true;
        }
        m_filters = Utils::splitFilterUiText(m_ui->filePattern->text());
        m_exclusionFilters = Utils::splitFilterUiText(m_ui->exclusionPattern->text());
        setShortcutString(m_ui->shortcutEdit->text().trimmed());
        setIncludedByDefault(m_ui->defaultFlag->isChecked());
        needsRefresh = directoriesChanged || oldFilters != m_filters
                || oldExclusionFilters != m_exclusionFilters;
        success = true;
    }
    return success;
}

void DirectoryFilter::handleAddDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Select Directory"));
    if (!dir.isEmpty())
        m_ui->directoryList->addItem(dir);
}

void DirectoryFilter::handleEditDirectory()
{
    if (m_ui->directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_ui->directoryList->selectedItems().at(0);
    QString dir = QFileDialog::getExistingDirectory(m_dialog, tr("Select Directory"),
                                                    currentItem->text());
    if (!dir.isEmpty())
        currentItem->setText(dir);
}

void DirectoryFilter::handleRemoveDirectory()
{
    if (m_ui->directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_ui->directoryList->selectedItems().at(0);
    delete m_ui->directoryList->takeItem(m_ui->directoryList->row(currentItem));
}

void DirectoryFilter::updateOptionButtons()
{
    bool haveSelectedItem = !m_ui->directoryList->selectedItems().isEmpty();
    m_ui->editButton->setEnabled(haveSelectedItem);
    m_ui->removeButton->setEnabled(haveSelectedItem);
}

void DirectoryFilter::updateFileIterator()
{
    QMutexLocker locker(&m_lock);
    setFileIterator(new BaseFileFilter::ListIterator(m_files));
}

void DirectoryFilter::refresh(QFutureInterface<void> &future)
{
    QStringList directories, filters, exclusionFilters;
    {
        QMutexLocker locker(&m_lock);
        if (m_directories.isEmpty()) {
            m_files.clear();
            QMetaObject::invokeMethod(this, &DirectoryFilter::updateFileIterator,
                                      Qt::QueuedConnection);
            future.setProgressRange(0, 1);
            future.setProgressValueAndText(1, tr("%1 filter update: 0 files").arg(displayName()));
            return;
        }
        directories = m_directories;
        filters = m_filters;
        exclusionFilters = m_exclusionFilters;
    }
    Utils::SubDirFileIterator subDirIterator(directories, filters, exclusionFilters);
    future.setProgressRange(0, subDirIterator.maxProgress());
    Utils::FilePaths filesFound;
    auto end = subDirIterator.end();
    for (auto it = subDirIterator.begin(); it != end; ++it) {
        if (future.isCanceled())
            break;
        filesFound << Utils::FilePath::fromString((*it).filePath);
        if (future.isProgressUpdateNeeded()
                || future.progressValue() == 0 /*workaround for regression in Qt*/) {
            future.setProgressValueAndText(subDirIterator.currentProgress(),
                                           tr("%1 filter update: %n files", nullptr, filesFound.size()).arg(displayName()));
        }
    }

    if (!future.isCanceled()) {
        QMutexLocker locker(&m_lock);
        m_files = filesFound;
        QMetaObject::invokeMethod(this, &DirectoryFilter::updateFileIterator, Qt::QueuedConnection);
        future.setProgressValue(subDirIterator.maxProgress());
    } else {
        future.setProgressValueAndText(subDirIterator.currentProgress(), tr("%1 filter update: canceled").arg(displayName()));
    }
}

void DirectoryFilter::setIsCustomFilter(bool value)
{
    m_isCustomFilter = value;
}

void DirectoryFilter::setDirectories(const QStringList &directories)
{
    if (directories == m_directories)
        return;
    {
        QMutexLocker locker(&m_lock);
        m_directories = directories;
    }
    Internal::Locator::instance()->refresh({this});
}

void DirectoryFilter::addDirectory(const QString &directory)
{
    setDirectories(m_directories + QStringList(directory));
}

void DirectoryFilter::removeDirectory(const QString &directory)
{
    QStringList directories = m_directories;
    directories.removeOne(directory);
    setDirectories(directories);
}

QStringList DirectoryFilter::directories() const
{
    return m_directories;
}

void DirectoryFilter::setFilters(const QStringList &filters)
{
    QMutexLocker locker(&m_lock);
    m_filters = filters;
}

void DirectoryFilter::setExclusionFilters(const QStringList &exclusionFilters)
{
    QMutexLocker locker(&m_lock);
    m_exclusionFilters = exclusionFilters;
}

} // namespace Core
