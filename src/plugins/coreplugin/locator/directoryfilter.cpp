// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "directoryfilter.h"

#include "locator.h"

#include <coreplugin/coreconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/filesearch.h>
#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

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
    setDescription(tr("Matches all files from a custom set of directories. Append \"+<number>\" or "
                      "\":<number>\" to jump to the given line number. Append another "
                      "\"+<number>\" or \":<number>\" to jump to the column number as well."));
}

void DirectoryFilter::saveState(QJsonObject &object) const
{
    QMutexLocker locker(&m_lock); // m_files is modified in other thread

    if (displayName() != defaultDisplayName())
        object.insert(kDisplayNameKey, displayName());
    if (!m_directories.isEmpty()) {
        object.insert(kDirectoriesKey,
                      QJsonArray::fromStringList(
                          Utils::transform(m_directories, &FilePath::toString)));
    }
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

static FilePaths toFilePaths(const QJsonArray &array)
{
    return Utils::transform(array.toVariantList(),
                            [](const QVariant &v) { return FilePath::fromString(v.toString()); });
}

void DirectoryFilter::restoreState(const QJsonObject &object)
{
    QMutexLocker locker(&m_lock);
    setDisplayName(object.value(kDisplayNameKey).toString(defaultDisplayName()));
    m_directories = toFilePaths(object.value(kDirectoriesKey).toArray());
    m_filters = toStringList(
        object.value(kFiltersKey).toArray(QJsonArray::fromStringList(kFiltersDefault)));
    m_files = FileUtils::toFilePathList(toStringList(object.value(kFilesKey).toArray()));
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
        m_files = FileUtils::toFilePathList(files);
        if (!in.atEnd()) // Qt Creator 4.3 and later
            in >> m_exclusionFilters;
        else
            m_exclusionFilters.clear();

        if (m_isCustomFilter) {
            m_directories = Utils::transform(directories, [](const QString &d) {
                return FilePath::fromString(d);
            });
        }
        setDisplayName(name);
        setShortcutString(shortcut);
        setIncludedByDefault(defaultFilter);

        locker.unlock();
    } else {
        ILocatorFilter::restoreState(state);
    }
    updateFileIterator();
}

class DirectoryFilterOptions : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Core::Internal::DirectoryFilterOptions)

public:
    DirectoryFilterOptions(QWidget *parent)
        : QDialog(parent)
    {
        nameLabel = new QLabel(tr("Name:"));
        nameEdit = new QLineEdit(this);

        filePatternLabel = new QLabel(this);
        filePattern = new QLineEdit(this);

        exclusionPatternLabel = new QLabel(this);
        exclusionPattern = new QLineEdit(this);

        prefixLabel = new QLabel(this);

        shortcutEdit = new QLineEdit(this);
        shortcutEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        shortcutEdit->setMaximumSize(QSize(16777215, 16777215));
        shortcutEdit->setToolTip(tr("Specify a short word/abbreviation that can be used to "
                                    "restrict completions to files from this directory tree.\n"
                                    "To do this, you type this shortcut and a space in the "
                                    "Locator entry field, and then the word to search for."));

        defaultFlag = new QCheckBox(this);
        defaultFlag->setChecked(false);

        directoryLabel = new QLabel(tr("Directories:"));

        addButton = new QPushButton(tr("Add..."));
        editButton = new QPushButton(tr("Edit..."));
        removeButton = new QPushButton(tr("Remove"));

        directoryList = new QListWidget(this);
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(1);
        sizePolicy1.setVerticalStretch(0);
        directoryList->setSizePolicy(sizePolicy1);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        using namespace Layouting;

        Column buttons {
            addButton,
            editButton,
            removeButton,
            st
        };

        Column {
            Grid {
                nameLabel, Span(3, nameEdit), br,
                Column { directoryLabel, st }, Span(2, directoryList), buttons, br,
                filePatternLabel, Span(3, filePattern), br,
                exclusionPatternLabel, Span(3, exclusionPattern), br,
                prefixLabel, shortcutEdit, Span(2, defaultFlag)
            },
            st,
            buttonBox
        }.attachTo(this);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QLabel *nameLabel;
    QLineEdit *nameEdit;
    QLabel *filePatternLabel;
    QLineEdit *filePattern;
    QLabel *exclusionPatternLabel;
    QLineEdit *exclusionPattern;
    QLabel *prefixLabel;
    QLineEdit *shortcutEdit;
    QCheckBox *defaultFlag;
    QPushButton *addButton;
    QPushButton *editButton;
    QPushButton *removeButton;
    QLabel *directoryLabel;
    QListWidget *directoryList;
};

bool DirectoryFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    bool success = false;

    DirectoryFilterOptions dialog(parent);
    m_dialog = &dialog;

    dialog.setWindowTitle(ILocatorFilter::msgConfigureDialogTitle());
    m_dialog->prefixLabel->setText(ILocatorFilter::msgPrefixLabel());
    m_dialog->prefixLabel->setToolTip(ILocatorFilter::msgPrefixToolTip());
    m_dialog->defaultFlag->setText(ILocatorFilter::msgIncludeByDefault());
    m_dialog->defaultFlag->setToolTip(ILocatorFilter::msgIncludeByDefaultToolTip());
    m_dialog->nameEdit->setText(displayName());
    m_dialog->nameEdit->selectAll();

    connect(m_dialog->addButton,
            &QPushButton::clicked,
            this,
            &DirectoryFilter::handleAddDirectory,
            Qt::DirectConnection);
    connect(m_dialog->editButton,
            &QPushButton::clicked,
            this,
            &DirectoryFilter::handleEditDirectory,
            Qt::DirectConnection);
    connect(m_dialog->removeButton,
            &QPushButton::clicked,
            this,
            &DirectoryFilter::handleRemoveDirectory,
            Qt::DirectConnection);
    connect(m_dialog->directoryList,
            &QListWidget::itemSelectionChanged,
            this,
            &DirectoryFilter::updateOptionButtons,
            Qt::DirectConnection);
    m_dialog->directoryList->clear();
    // Note: assuming we only change m_directories in the Gui thread,
    // we don't need to protect it here with mutex
    m_dialog->directoryList->addItems(Utils::transform(m_directories, &FilePath::toString));
    m_dialog->nameLabel->setVisible(m_isCustomFilter);
    m_dialog->nameEdit->setVisible(m_isCustomFilter);
    m_dialog->directoryLabel->setVisible(m_isCustomFilter);
    m_dialog->directoryList->setVisible(m_isCustomFilter);
    m_dialog->addButton->setVisible(m_isCustomFilter);
    m_dialog->editButton->setVisible(m_isCustomFilter);
    m_dialog->removeButton->setVisible(m_isCustomFilter);
    m_dialog->filePatternLabel->setText(Utils::msgFilePatternLabel());
    m_dialog->filePatternLabel->setBuddy(m_dialog->filePattern);
    m_dialog->filePattern->setToolTip(Utils::msgFilePatternToolTip());
    // Note: assuming we only change m_filters in the Gui thread,
    // we don't need to protect it here with mutex
    m_dialog->filePattern->setText(Utils::transform(m_filters, &QDir::toNativeSeparators).join(','));
    m_dialog->exclusionPatternLabel->setText(Utils::msgExclusionPatternLabel());
    m_dialog->exclusionPatternLabel->setBuddy(m_dialog->exclusionPattern);
    m_dialog->exclusionPattern->setToolTip(Utils::msgFilePatternToolTip());
    // Note: assuming we only change m_exclusionFilters in the Gui thread,
    // we don't need to protect it here with mutex
    m_dialog->exclusionPattern->setText(
        Utils::transform(m_exclusionFilters, &QDir::toNativeSeparators).join(','));
    m_dialog->shortcutEdit->setText(shortcutString());
    m_dialog->defaultFlag->setChecked(isIncludedByDefault());
    updateOptionButtons();
    dialog.adjustSize();
    if (dialog.exec() == QDialog::Accepted) {
        QMutexLocker locker(&m_lock);
        bool directoriesChanged = false;
        const FilePaths oldDirectories = m_directories;
        const QStringList oldFilters = m_filters;
        const QStringList oldExclusionFilters = m_exclusionFilters;
        setDisplayName(m_dialog->nameEdit->text().trimmed());
        m_directories.clear();
        const int oldCount = oldDirectories.count();
        const int newCount = m_dialog->directoryList->count();
        if (oldCount != newCount)
            directoriesChanged = true;
        for (int i = 0; i < newCount; ++i) {
            m_directories.append(FilePath::fromString(m_dialog->directoryList->item(i)->text()));
            if (!directoriesChanged && m_directories.at(i) != oldDirectories.at(i))
                directoriesChanged = true;
        }
        m_filters = Utils::splitFilterUiText(m_dialog->filePattern->text());
        m_exclusionFilters = Utils::splitFilterUiText(m_dialog->exclusionPattern->text());
        setShortcutString(m_dialog->shortcutEdit->text().trimmed());
        setIncludedByDefault(m_dialog->defaultFlag->isChecked());
        needsRefresh = directoriesChanged || oldFilters != m_filters
                || oldExclusionFilters != m_exclusionFilters;
        success = true;
    }
    return success;
}

void DirectoryFilter::handleAddDirectory()
{
    FilePath dir = FileUtils::getExistingDirectory(m_dialog, tr("Select Directory"));
    if (!dir.isEmpty())
        m_dialog->directoryList->addItem(dir.toUserOutput());
}

void DirectoryFilter::handleEditDirectory()
{
    if (m_dialog->directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_dialog->directoryList->selectedItems().at(0);
    FilePath dir = FileUtils::getExistingDirectory(m_dialog, tr("Select Directory"),
                                                   FilePath::fromUserInput(currentItem->text()));
    if (!dir.isEmpty())
        currentItem->setText(dir.toUserOutput());
}

void DirectoryFilter::handleRemoveDirectory()
{
    if (m_dialog->directoryList->selectedItems().count() < 1)
        return;
    QListWidgetItem *currentItem = m_dialog->directoryList->selectedItems().at(0);
    delete m_dialog->directoryList->takeItem(m_dialog->directoryList->row(currentItem));
}

void DirectoryFilter::updateOptionButtons()
{
    bool haveSelectedItem = !m_dialog->directoryList->selectedItems().isEmpty();
    m_dialog->editButton->setEnabled(haveSelectedItem);
    m_dialog->removeButton->setEnabled(haveSelectedItem);
}

void DirectoryFilter::updateFileIterator()
{
    QMutexLocker locker(&m_lock);
    setFileIterator(new BaseFileFilter::ListIterator(m_files));
}

void DirectoryFilter::refresh(QFutureInterface<void> &future)
{
    FilePaths directories;
    QStringList filters, exclusionFilters;
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
        filesFound << (*it).filePath;
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

void DirectoryFilter::setDirectories(const FilePaths &directories)
{
    if (directories == m_directories)
        return;
    {
        QMutexLocker locker(&m_lock);
        m_directories = directories;
    }
    Internal::Locator::instance()->refresh({this});
}

void DirectoryFilter::addDirectory(const FilePath &directory)
{
    setDirectories(m_directories + FilePaths{directory});
}

void DirectoryFilter::removeDirectory(const FilePath &directory)
{
    FilePaths directories = m_directories;
    directories.removeOne(directory);
    setDirectories(directories);
}

FilePaths DirectoryFilter::directories() const
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
