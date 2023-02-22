// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemfilter.h"

#include "../coreplugintr.h"
#include "../documentmanager.h"
#include "../editormanager/editormanager.h"
#include "../icore.h"
#include "../vcsmanager.h"

#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>

using namespace Utils;

namespace Core {
namespace Internal {

ILocatorFilter::MatchLevel FileSystemFilter::matchLevelFor(const QRegularExpressionMatch &match,
                                                           const QString &matchText)
{
    const int consecutivePos = match.capturedStart(1);
    if (consecutivePos == 0)
        return MatchLevel::Best;
    if (consecutivePos > 0) {
        const QChar prevChar = matchText.at(consecutivePos - 1);
        if (prevChar == '_' || prevChar == '.')
            return MatchLevel::Better;
    }
    if (match.capturedStart() == 0)
        return MatchLevel::Good;
    return MatchLevel::Normal;
}

FileSystemFilter::FileSystemFilter()
{
    setId("Files in file system");
    setDisplayName(Tr::tr("Files in File System"));
    setDescription(Tr::tr("Opens a file given by a relative path to the current document, or absolute "
                      "path. \"~\" refers to your home directory. You have the option to create a "
                      "file if it does not exist yet."));
    setDefaultShortcutString("f");
    setDefaultIncludedByDefault(false);
}

void FileSystemFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_currentDocumentDirectory = DocumentManager::fileDialogInitialDirectory();
    m_currentIncludeHidden = m_includeHidden;
}

QList<LocatorFilterEntry> FileSystemFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                       const QString &entry)
{
    QList<LocatorFilterEntry> entries[int(MatchLevel::Count)];

    Environment env = Environment::systemEnvironment();
    const QString expandedEntry = env.expandVariables(entry);
    const auto expandedEntryPath = FilePath::fromUserInput(expandedEntry);
    const auto absoluteEntryPath = m_currentDocumentDirectory.isEmpty()
                                       ? expandedEntryPath
                                       : m_currentDocumentDirectory.resolvePath(expandedEntryPath);

    // Consider the entered path a directory if it ends with slash/backslash.
    // If it is a dir but doesn't end with a backslash, we want to still show all (other) matching
    // items from the same parent directory.
    // Unfortunately fromUserInput removes slash/backslash at the end, so manually check the original.
    const bool isDir = expandedEntry.isEmpty() || expandedEntry.endsWith('/')
                       || expandedEntry.endsWith('\\');
    const FilePath directory = isDir ? absoluteEntryPath : absoluteEntryPath.parentDir();
    const QString entryFileName = isDir ? QString() : absoluteEntryPath.fileName();

    QDir::Filters dirFilter = QDir::Dirs | QDir::Drives | QDir::NoDot | QDir::NoDotDot;
    QDir::Filters fileFilter = QDir::Files;
    if (m_currentIncludeHidden) {
        dirFilter |= QDir::Hidden;
        fileFilter |= QDir::Hidden;
    }
    // use only 'name' for case sensitivity decision, because we need to make the path
    // match the case on the file system for case-sensitive file systems
    const Qt::CaseSensitivity caseSensitivity_ = caseSensitivity(entryFileName);
    const FilePaths dirs = FilePaths({directory / ".."})
                           + directory.dirEntries({{}, dirFilter},
                                                  QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);
    const FilePaths files = directory.dirEntries({{}, fileFilter},
                                                 QDir::Name | QDir::IgnoreCase | QDir::LocaleAware);

    QRegularExpression regExp = createRegExp(entryFileName, caseSensitivity_);
    if (!regExp.isValid())
        return {};

    for (const FilePath &dir : dirs) {
        if (future.isCanceled())
            break;

        const QString dirString = dir.relativeChildPath(directory).nativePath();
        const QRegularExpressionMatch match = regExp.match(dirString);
        if (match.hasMatch()) {
            const MatchLevel level = matchLevelFor(match, dirString);
            LocatorFilterEntry filterEntry(this, dirString);
            filterEntry.filePath = dir;
            filterEntry.highlightInfo = highlightInfo(match);

            entries[int(level)].append(filterEntry);
        }
    }
    // file names can match with +linenumber or :linenumber
    const Link link = Link::fromString(entryFileName, true);
    regExp = createRegExp(link.targetFilePath.toString(), caseSensitivity_);
    if (!regExp.isValid())
        return {};
    for (const FilePath &file : files) {
        if (future.isCanceled())
            break;

        const QString fileString = file.relativeChildPath(directory).nativePath();
        const QRegularExpressionMatch match = regExp.match(fileString);
        if (match.hasMatch()) {
            const MatchLevel level = matchLevelFor(match, fileString);
            LocatorFilterEntry filterEntry(this, fileString);
            filterEntry.filePath = file;
            filterEntry.highlightInfo = highlightInfo(match);
            filterEntry.linkForEditor = Link(filterEntry.filePath, link.targetLine,
                                             link.targetColumn);
            entries[int(level)].append(filterEntry);
        }
    }

    // "create and open" functionality
    const FilePath fullFilePath = directory / entryFileName;
    const bool containsWildcard = expandedEntry.contains('?') || expandedEntry.contains('*');
    if (!containsWildcard && !fullFilePath.exists() && directory.exists()) {
        LocatorFilterEntry createAndOpen(this, Tr::tr("Create and Open \"%1\"").arg(expandedEntry));
        createAndOpen.filePath = fullFilePath;
        createAndOpen.extraInfo = directory.absoluteFilePath().shortNativePath();
        entries[int(MatchLevel::Normal)].append(createAndOpen);
    }

    return std::accumulate(std::begin(entries), std::end(entries), QList<LocatorFilterEntry>());
}

const char kAlwaysCreate[] = "Locator/FileSystemFilter/AlwaysCreate";

void FileSystemFilter::accept(const LocatorFilterEntry &selection,
                              QString *newText,
                              int *selectionStart,
                              int *selectionLength) const
{
    Q_UNUSED(selectionLength)
    if (selection.filePath.isDir()) {
        const QString value
            = shortcutString() + ' '
              + selection.filePath.absoluteFilePath().cleanPath().pathAppended("/").toUserOutput();
        *newText = value;
        *selectionStart = value.length();
    } else {
        // Don't block locator filter execution with dialog
        QMetaObject::invokeMethod(EditorManager::instance(), [selection] {
            if (!selection.filePath.exists()) {
                if (CheckableMessageBox::shouldAskAgain(ICore::settings(), kAlwaysCreate)) {
                    CheckableMessageBox messageBox(ICore::dialogParent());
                    messageBox.setWindowTitle(Tr::tr("Create File"));
                    messageBox.setIcon(QMessageBox::Question);
                    messageBox.setText(Tr::tr("Create \"%1\"?").arg(selection.filePath.shortNativePath()));
                    messageBox.setCheckBoxVisible(true);
                    messageBox.setCheckBoxText(Tr::tr("Always create"));
                    messageBox.setChecked(false);
                    messageBox.setStandardButtons(QDialogButtonBox::Cancel);
                    QPushButton *createButton = messageBox.addButton(Tr::tr("Create"),
                                                                     QDialogButtonBox::AcceptRole);
                    messageBox.setDefaultButton(QDialogButtonBox::Cancel);
                    messageBox.exec();
                    if (messageBox.clickedButton() != createButton)
                        return;
                    if (messageBox.isChecked())
                        CheckableMessageBox::doNotAskAgain(ICore::settings(), kAlwaysCreate);
                }
                QFile file(selection.filePath.toFSPathString());
                file.open(QFile::WriteOnly);
                file.close();
                VcsManager::promptToAdd(selection.filePath.absolutePath(), {selection.filePath});
            }
            EditorManager::openEditor(selection);
        }, Qt::QueuedConnection);
    }
}

class FileSystemFilterOptions : public QDialog
{
public:
    FileSystemFilterOptions(QWidget *parent)
        : QDialog(parent)
    {
        resize(360, 131);
        setWindowTitle(ILocatorFilter::msgConfigureDialogTitle());

        auto prefixLabel = new QLabel;
        prefixLabel->setText(ILocatorFilter::msgPrefixLabel());
        prefixLabel->setToolTip(ILocatorFilter::msgPrefixToolTip());

        shortcutEdit = new QLineEdit;
        includeByDefault = new QCheckBox;
        hiddenFilesFlag = new QCheckBox(Tr::tr("Include hidden files"));

        prefixLabel->setBuddy(shortcutEdit);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        using namespace Layouting;
        Column {
            Grid {
                prefixLabel, shortcutEdit, includeByDefault, br,
                Tr::tr("Filter:"), hiddenFilesFlag, br,
            },
            st,
            Row {st, buttonBox }
        }.attachTo(this);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QLineEdit *shortcutEdit;
    QCheckBox *includeByDefault;
    QCheckBox *hiddenFilesFlag;
};

bool FileSystemFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    FileSystemFilterOptions dialog(parent);
    dialog.includeByDefault->setText(msgIncludeByDefault());
    dialog.includeByDefault->setToolTip(msgIncludeByDefaultToolTip());
    dialog.includeByDefault->setChecked(isIncludedByDefault());
    dialog.hiddenFilesFlag->setChecked(m_includeHidden);
    dialog.shortcutEdit->setText(shortcutString());

    if (dialog.exec() == QDialog::Accepted) {
        m_includeHidden = dialog.hiddenFilesFlag->isChecked();
        setShortcutString(dialog.shortcutEdit->text().trimmed());
        setIncludedByDefault(dialog.includeByDefault->isChecked());
        return true;
    }
    return false;
}

const char kIncludeHiddenKey[] = "includeHidden";

void FileSystemFilter::saveState(QJsonObject &object) const
{
    if (m_includeHidden != kIncludeHiddenDefault)
        object.insert(kIncludeHiddenKey, m_includeHidden);
}

void FileSystemFilter::restoreState(const QJsonObject &object)
{
    m_currentIncludeHidden = object.value(kIncludeHiddenKey).toBool(kIncludeHiddenDefault);
}

void FileSystemFilter::restoreState(const QByteArray &state)
{
    if (isOldSetting(state)) {
        // TODO read old settings, remove some time after Qt Creator 4.15
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
    } else {
        ILocatorFilter::restoreState(state);
    }
}

} // Internal
} // Core
