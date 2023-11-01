// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filesystemfilter.h"

#include "../coreplugintr.h"
#include "../documentmanager.h"
#include "../editormanager/editormanager.h"
#include "../icore.h"
#include "../vcsmanager.h"
#include "locatormanager.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyle>

using namespace Utils;

namespace Core::Internal {

Q_GLOBAL_STATIC(QIcon, sDeviceRootIcon);

static const char kAlwaysCreate[] = "Locator/FileSystemFilter/AlwaysCreate";

static ILocatorFilter::MatchLevel matchLevelFor(const QRegularExpressionMatch &match,
                                                const QString &matchText)
{
    const int consecutivePos = match.capturedStart(1);
    if (consecutivePos == 0)
        return ILocatorFilter::MatchLevel::Best;
    if (consecutivePos > 0) {
        const QChar prevChar = matchText.at(consecutivePos - 1);
        if (prevChar == '_' || prevChar == '.')
            return ILocatorFilter::MatchLevel::Better;
    }
    if (match.capturedStart() == 0)
        return ILocatorFilter::MatchLevel::Good;
    return ILocatorFilter::MatchLevel::Normal;
}

static bool askForCreating(const QString &title, const FilePath &filePath)
{
    QMessageBox::StandardButton selected
        = CheckableMessageBox::question(ICore::dialogParent(),
                                        title,
                                        Tr::tr("Create \"%1\"?").arg(filePath.shortNativePath()),
                                        Key(kAlwaysCreate),
                                        QMessageBox::Yes | QMessageBox::Cancel,
                                        QMessageBox::Cancel,
                                        QMessageBox::Yes,
                                        {{QMessageBox::Yes, Tr::tr("Create")}},
                                        Tr::tr("Always create"));
    return selected == QMessageBox::Yes;
}

static void createAndOpenFile(const FilePath &filePath)
{
    if (!filePath.exists()) {
        if (askForCreating(Tr::tr("Create File"), filePath)) {
            QFile file(filePath.toFSPathString());
            file.open(QFile::WriteOnly);
            file.close();
            VcsManager::promptToAdd(filePath.absolutePath(), {filePath});
        }
    }
    if (filePath.exists())
        EditorManager::openEditor(filePath);
}

static bool createDirectory(const FilePath &filePath)
{
    if (!filePath.exists()) {
        if (askForCreating(Tr::tr("Create Directory"), filePath))
            filePath.createDir();
    }
    if (filePath.exists())
        return true;
    return false;
}

static FilePaths deviceRoots()
{
    const QString rootPath = FilePath::specialRootPath();
    const QStringList roots = QDir(rootPath).entryList();
    FilePaths devices;
    for (const QString &root : roots) {
        const QString prefix = rootPath + '/' + root;
        devices += Utils::transform(QDir(prefix).entryList(), [prefix](const QString &s) {
            return FilePath::fromString(prefix + '/' + s);
        });
    }
    return devices;
}

FileSystemFilter::FileSystemFilter()
{
    setId("Files in file system");
    setDisplayName(Tr::tr("Files in File System"));
    setDescription(Tr::tr("Opens a file given by a relative path to the current document, or absolute "
                      "path. \"~\" refers to your home directory. You have the option to create a "
                      "file if it does not exist yet."));
    setDefaultShortcutString("f");
    *sDeviceRootIcon = qApp->style()->standardIcon(QStyle::SP_DriveHDIcon);
}

static void matches(QPromise<void> &promise, const LocatorStorage &storage,
                    const QString &shortcutString, const FilePath &currentDocumentDir,
                    bool includeHidden)
{
    const QString input = storage.input();
    LocatorFilterEntries entries[int(ILocatorFilter::MatchLevel::Count)];

    const Environment env = Environment::systemEnvironment();
    const QString expandedEntry = env.expandVariables(input);
    const auto expandedEntryPath = FilePath::fromUserInput(expandedEntry);
    const FilePath absoluteEntryPath = currentDocumentDir.isEmpty()
                                           ? expandedEntryPath
                                           : currentDocumentDir.resolvePath(expandedEntryPath);
    // The case of e.g. "ssh://", "ssh://*p", etc
    const bool isPartOfDeviceRoot = expandedEntryPath.needsDevice()
                                    && expandedEntryPath.path().isEmpty();

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
    if (includeHidden) {
        dirFilter |= QDir::Hidden;
        fileFilter |= QDir::Hidden;
    }
    // use only 'name' for case sensitivity decision, because we need to make the path
    // match the case on the file system for case-sensitive file systems
    const Qt::CaseSensitivity caseSensitivity = ILocatorFilter::caseSensitivity(entryFileName);
    const FilePaths dirs = isPartOfDeviceRoot
                               ? FilePaths()
                               : FilePaths({directory / ".."})
                                     + directory.dirEntries({{}, dirFilter},
                                                            QDir::Name | QDir::IgnoreCase
                                                                | QDir::LocaleAware);
    const FilePaths files = isPartOfDeviceRoot ? FilePaths()
                                               : directory.dirEntries({{}, fileFilter},
                                                                      QDir::Name | QDir::IgnoreCase
                                                                          | QDir::LocaleAware);

    // directories
    QRegularExpression regExp = ILocatorFilter::createRegExp(entryFileName, caseSensitivity);
    if (regExp.isValid()) {
        for (const FilePath &dir : dirs) {
            if (promise.isCanceled())
                return;

            const QString dirString = dir.relativeChildPath(directory).nativePath();
            const QRegularExpressionMatch match = regExp.match(dirString);
            if (match.hasMatch()) {
                const ILocatorFilter::MatchLevel level = matchLevelFor(match, dirString);
                LocatorFilterEntry filterEntry;
                filterEntry.displayName = dirString;
                filterEntry.acceptor = [shortcutString, dir] {
                    const QString value
                        = shortcutString + ' '
                          + dir.absoluteFilePath().cleanPath().pathAppended("/").toUserOutput();
                    return AcceptResult{value, int(value.length())};
                };
                filterEntry.filePath = dir;
                filterEntry.highlightInfo = ILocatorFilter::highlightInfo(match);

                entries[int(level)].append(filterEntry);
            }
        }
    }
    // file names can match with +linenumber or :linenumber
    const Link link = Link::fromString(entryFileName, true);
    regExp = ILocatorFilter::createRegExp(link.targetFilePath.toString(), caseSensitivity);
    if (regExp.isValid()) {
        for (const FilePath &file : files) {
            if (promise.isCanceled())
                return;

            const QString fileString = file.relativeChildPath(directory).nativePath();
            const QRegularExpressionMatch match = regExp.match(fileString);
            if (match.hasMatch()) {
                const ILocatorFilter::MatchLevel level = matchLevelFor(match, fileString);
                LocatorFilterEntry filterEntry;
                filterEntry.displayName = fileString;
                filterEntry.filePath = file;
                filterEntry.highlightInfo = ILocatorFilter::highlightInfo(match);
                filterEntry.linkForEditor = Link(filterEntry.filePath,
                                                 link.targetLine,
                                                 link.targetColumn);
                entries[int(level)].append(filterEntry);
            }
        }
    }
    // device roots
    // check against full search text
    regExp = ILocatorFilter::createRegExp(expandedEntryPath.toUserOutput(), caseSensitivity);
    if (regExp.isValid()) {
        const FilePaths roots = deviceRoots();
        for (const FilePath &root : roots) {
            if (promise.isCanceled())
                return;

            const QString displayString = root.toUserOutput();
            const QRegularExpressionMatch match = regExp.match(displayString);
            if (match.hasMatch()) {
                LocatorFilterEntry filterEntry;
                filterEntry.displayName = displayString;
                filterEntry.acceptor = [shortcutString, root] {
                    const QString value
                        = shortcutString + ' '
                          + root.absoluteFilePath().cleanPath().pathAppended("/").toUserOutput();
                    return AcceptResult{value, int(value.length())};
                };
                filterEntry.filePath = root;
                filterEntry.displayIcon = *sDeviceRootIcon;
                filterEntry.highlightInfo = ILocatorFilter::highlightInfo(match);

                entries[int(ILocatorFilter::MatchLevel::Normal)].append(filterEntry);
            }
        }
    }

    // "create and open" functionality
    const FilePath fullFilePath = directory / entryFileName;
    const bool containsWildcard = expandedEntry.contains('?') || expandedEntry.contains('*');
    if (!containsWildcard && !fullFilePath.exists() && directory.exists()) {
        // create and open file
        {
            LocatorFilterEntry filterEntry;
            filterEntry.displayName = Tr::tr("Create and Open File \"%1\"").arg(expandedEntry);
            filterEntry.displayIcon = Utils::FileIconProvider::icon(QFileIconProvider::File);
            filterEntry.acceptor = [fullFilePath] {
                QMetaObject::invokeMethod(
                    EditorManager::instance(),
                    [fullFilePath] { createAndOpenFile(fullFilePath); },
                    Qt::QueuedConnection);
                return AcceptResult();
            };
            filterEntry.filePath = fullFilePath;
            filterEntry.extraInfo = directory.absoluteFilePath().shortNativePath();
            entries[int(ILocatorFilter::MatchLevel::Normal)].append(filterEntry);
        }

        // create directory
        {
            LocatorFilterEntry filterEntry;
            filterEntry.displayName = Tr::tr("Create Directory \"%1\"").arg(expandedEntry);
            filterEntry.displayIcon = Utils::FileIconProvider::icon(QFileIconProvider::Folder);
            filterEntry.acceptor = [fullFilePath, shortcutString] {
                QMetaObject::invokeMethod(
                    EditorManager::instance(),
                    [fullFilePath, shortcutString] {
                        if (createDirectory(fullFilePath)) {
                            const QString value = shortcutString + ' '
                                                  + fullFilePath.absoluteFilePath()
                                                        .cleanPath()
                                                        .pathAppended("/")
                                                        .toUserOutput();
                            LocatorManager::show(value, value.length());
                        }
                    },
                    Qt::QueuedConnection);
                return AcceptResult();
            };
            filterEntry.filePath = fullFilePath;
            filterEntry.extraInfo = directory.absoluteFilePath().shortNativePath();
            entries[int(ILocatorFilter::MatchLevel::Normal)].append(filterEntry);
        }
    }

    storage.reportOutput(std::accumulate(std::begin(entries), std::end(entries),
                                         LocatorFilterEntries()));
}

LocatorMatcherTasks FileSystemFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage, includeHidden = m_includeHidden, shortcut = shortcutString()]
        (Async<void> &async) {
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(matches, *storage, shortcut,
                                    DocumentManager::fileDialogInitialDirectory(), includeHidden);
    };

    return {{AsyncTask<void>(onSetup), storage}};
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
    if (m_includeHidden != s_includeHiddenDefault)
        object.insert(kIncludeHiddenKey, m_includeHidden);
}

void FileSystemFilter::restoreState(const QJsonObject &object)
{
    m_includeHidden = object.value(kIncludeHiddenKey).toBool(s_includeHiddenDefault);
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

} // namespace Core::Internal
