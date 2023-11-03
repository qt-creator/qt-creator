// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileutils.h"
#include "savefile.h"

#include "algorithm.h"
#include "devicefileaccess.h"
#include "environment.h"
#include "qtcassert.h"
#include "utilstr.h"

#include "fsengine/fileiconprovider.h"
#include "fsengine/fsengine.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QTextStream>
#include <QXmlStreamWriter>

#include <qplatformdefs.h>

#ifdef QT_GUI_LIB
#include <QMessageBox>
#include <QGuiApplication>
#endif

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#include <shlobj.h>
#endif

#ifdef Q_OS_MACOS
#include "fileutils_mac.h"
#endif

namespace Utils {

// FileReader

QByteArray FileReader::fetchQrc(const QString &fileName)
{
    QTC_ASSERT(fileName.startsWith(':'), return QByteArray());
    QFile file(fileName);
    bool ok = file.open(QIODevice::ReadOnly);
    QTC_ASSERT(ok, qWarning() << fileName << "not there!"; return QByteArray());
    return file.readAll();
}

bool FileReader::fetch(const FilePath &filePath, QIODevice::OpenMode mode)
{
    QTC_ASSERT(!(mode & ~(QIODevice::ReadOnly | QIODevice::Text)), return false);

    const expected_str<QByteArray> contents = filePath.fileContents();
    if (!contents) {
        m_errorString = contents.error();
        return false;
    }
    m_data = *contents;

    if (mode & QIODevice::Text)
        m_data = m_data.replace("\r\n", "\n");

    return true;
}

bool FileReader::fetch(const FilePath &filePath, QIODevice::OpenMode mode, QString *errorString)
{
    if (fetch(filePath, mode))
        return true;
    if (errorString)
        *errorString = m_errorString;
    return false;
}

#ifdef QT_GUI_LIB
bool FileReader::fetch(const FilePath &filePath, QIODevice::OpenMode mode, QWidget *parent)
{
    if (fetch(filePath, mode))
        return true;
    if (parent)
        QMessageBox::critical(parent, Tr::tr("File Error"), m_errorString);
    return false;
}
#endif // QT_GUI_LIB

// FileSaver

FileSaverBase::FileSaverBase() = default;

FileSaverBase::~FileSaverBase() = default;

bool FileSaverBase::finalize()
{
    m_file->close();
    setResult(m_file->error() == QFile::NoError);
    m_file.reset();
    return !m_hasError;
}

bool FileSaverBase::finalize(QString *errStr)
{
    if (finalize())
        return true;
    if (errStr)
        *errStr = errorString();
    return false;
}

#ifdef QT_GUI_LIB
bool FileSaverBase::finalize(QWidget *parent)
{
    if (finalize())
        return true;
    QMessageBox::critical(parent, Tr::tr("File Error"), errorString());
    return false;
}
#endif // QT_GUI_LIB

bool FileSaverBase::write(const char *data, int len)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(data, len) == len);
}

bool FileSaverBase::write(const QByteArray &bytes)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(bytes) == bytes.size());
}

bool FileSaverBase::setResult(bool ok)
{
    if (!ok && !m_hasError) {
        if (!m_file->errorString().isEmpty()) {
            m_errorString = Tr::tr("Cannot write file %1: %2")
                                .arg(m_filePath.toUserOutput(), m_file->errorString());
        } else {
            m_errorString = Tr::tr("Cannot write file %1. Disk full?")
                                .arg(m_filePath.toUserOutput());
        }
        m_hasError = true;
    }
    return ok;
}

bool FileSaverBase::setResult(QTextStream *stream)
{
    stream->flush();
    return setResult(stream->status() == QTextStream::Ok);
}

bool FileSaverBase::setResult(QDataStream *stream)
{
    return setResult(stream->status() == QDataStream::Ok);
}

bool FileSaverBase::setResult(QXmlStreamWriter *stream)
{
    return setResult(!stream->hasError());
}

// FileSaver

FileSaver::FileSaver(const FilePath &filePath, QIODevice::OpenMode mode)
{
    m_filePath = filePath;
    // Workaround an assert in Qt -- and provide a useful error message, too:
    if (m_filePath.osType() == OsType::OsTypeWindows) {
        // Taken from: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
        static const QStringList reservedNames
                = {"CON", "PRN", "AUX", "NUL",
                   "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                   "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
        const QString fn = filePath.baseName().toUpper();
        if (reservedNames.contains(fn)) {
            m_errorString = Tr::tr("%1: Is a reserved filename on Windows. Cannot save.")
                                .arg(filePath.toUserOutput());
            m_hasError = true;
            return;
        }
    }
    if (filePath.needsDevice()) {
        // Write to a local temporary file first. Actual saving to the selected location
        // is done via m_filePath.writeFileContents() in finalize()
        m_isSafe = false;
        auto tf = new QTemporaryFile(QDir::tempPath() + "/remotefilesaver-XXXXXX");
        tf->setAutoRemove(false);
        m_file.reset(tf);
    } else {
        const bool readOnlyOrAppend = mode & (QIODevice::ReadOnly | QIODevice::Append);
        m_isSafe = !readOnlyOrAppend && !filePath.hasHardLinks();
        if (m_isSafe)
            m_file.reset(new SaveFile(filePath));
        else
            m_file.reset(new QFile{filePath.path()});
    }
    if (!m_file->open(QIODevice::WriteOnly | mode)) {
        QString err = filePath.exists() ?
                Tr::tr("Cannot overwrite file %1: %2") : Tr::tr("Cannot create file %1: %2");
        m_errorString = err.arg(filePath.toUserOutput(), m_file->errorString());
        m_hasError = true;
    }
}

bool FileSaver::finalize()
{
    if (m_filePath.needsDevice()) {
        m_file->close();
        m_file->open(QIODevice::ReadOnly);
        const QByteArray data = m_file->readAll();
        const expected_str<qint64> res = m_filePath.writeFileContents(data);
        m_file->remove();
        m_file.reset();
        return res.has_value();
    }

    if (!m_isSafe)
        return FileSaverBase::finalize();

    auto sf = static_cast<SaveFile *>(m_file.get());
    if (m_hasError) {
        if (sf->isOpen())
            sf->rollback();
    } else {
        setResult(sf->commit());
    }
    m_file.reset();
    return !m_hasError;
}

TempFileSaver::TempFileSaver(const QString &templ)
{
    m_file.reset(new QTemporaryFile{});
    auto tempFile = static_cast<QTemporaryFile *>(m_file.get());
    if (!templ.isEmpty())
        tempFile->setFileTemplate(templ);
    tempFile->setAutoRemove(false);
    if (!tempFile->open()) {
        m_errorString = Tr::tr("Cannot create temporary file in %1: %2").arg(
                QDir::toNativeSeparators(QFileInfo(tempFile->fileTemplate()).absolutePath()),
                tempFile->errorString());
        m_hasError = true;
    }
    m_filePath = FilePath::fromString(tempFile->fileName());
}

TempFileSaver::~TempFileSaver()
{
    m_file.reset();
    if (m_autoRemove)
        QFile::remove(m_filePath.toString());
}

/*!
    \class Utils::FileUtils
    \inmodule QtCreator

  \brief The FileUtils class contains file and directory related convenience
  functions.

*/

#ifdef QT_GUI_LIB
FileUtils::CopyAskingForOverwrite::CopyAskingForOverwrite(QWidget *dialogParent, const std::function<void (FilePath)> &postOperation)
    : m_parent(dialogParent)
    , m_postOperation(postOperation)
{}

bool FileUtils::CopyAskingForOverwrite::operator()(const FilePath &src,
                                                   const FilePath &dest,
                                                   QString *error)
{
    bool copyFile = true;
    if (dest.exists()) {
        if (m_skipAll)
            copyFile = false;
        else if (!m_overwriteAll) {
            const int res = QMessageBox::question(
                m_parent,
                Tr::tr("Overwrite File?"),
                Tr::tr("Overwrite existing file \"%1\"?").arg(dest.toUserOutput()),
                QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::NoToAll
                    | QMessageBox::Cancel);
            if (res == QMessageBox::Cancel) {
                return false;
            } else if (res == QMessageBox::No) {
                copyFile = false;
            } else if (res == QMessageBox::NoToAll) {
                m_skipAll = true;
                copyFile = false;
            } else if (res == QMessageBox::YesToAll) {
                m_overwriteAll = true;
            }
            if (copyFile)
                dest.removeFile();
        }
    }
    if (copyFile) {
        dest.parentDir().ensureWritableDir();
        if (!src.copyFile(dest)) {
            if (error) {
                *error = Tr::tr("Could not copy file \"%1\" to \"%2\".")
                             .arg(src.toUserOutput(), dest.toUserOutput());
            }
            return false;
        }
        if (m_postOperation)
            m_postOperation(dest);
    }
    m_files.append(dest.absoluteFilePath());
    return true;
}

FilePaths FileUtils::CopyAskingForOverwrite::files() const
{
    return m_files;
}
#endif // QT_GUI_LIB

FilePath FileUtils::commonPath(const FilePaths &paths)
{
    if (paths.isEmpty())
        return {};

    if (paths.count() == 1)
        return paths.constFirst();

    const FilePath &first = paths.constFirst();
    const FilePaths others = paths.mid(1);
    FilePath result;

    // Common scheme
    const QStringView commonScheme = first.scheme();
    auto sameScheme = [&commonScheme] (const FilePath &fp) {
        return commonScheme == fp.scheme();
    };
    if (!allOf(others, sameScheme))
        return result;
    result.setParts(commonScheme, {}, {});

    // Common host
    const QStringView commonHost = first.host();
    auto sameHost = [&commonHost] (const FilePath &fp) {
        return commonHost == fp.host();
    };
    if (!allOf(others, sameHost))
        return result;
    result.setParts(commonScheme, commonHost, {});

    // Common path
    QString commonPath;
    auto sameBasePath = [&commonPath] (const FilePath &fp) {
        return QString(fp.path() + '/').startsWith(commonPath);
    };
    const QStringList pathSegments = first.path().split('/');
    for (const QString &segment : pathSegments) {
        commonPath += segment + '/';
        if (!allOf(others, sameBasePath))
            return result;
        result.setParts(commonScheme, commonHost, commonPath.chopped(1));
    }

    return result;
}

#ifdef Q_OS_WIN
template <>
void withNtfsPermissions(const std::function<void()> &task)
{
    qt_ntfs_permission_lookup++;
    task();
    qt_ntfs_permission_lookup--;
}
#endif


#ifdef QT_WIDGETS_LIB

static std::function<QWidget *()> s_dialogParentGetter;

void FileUtils::setDialogParentGetter(const std::function<QWidget *()> &getter)
{
    s_dialogParentGetter = getter;
}

static QWidget *dialogParent(QWidget *parent)
{
    return parent ? parent : s_dialogParentGetter ? s_dialogParentGetter() : nullptr;
}

static FilePath qUrlToFilePath(const QUrl &url)
{
    if (url.isLocalFile())
        return FilePath::fromString(url.toLocalFile());
    return FilePath::fromParts(url.scheme(), url.host(), url.path());
}

static QUrl filePathToQUrl(const FilePath &filePath)
{
    return QUrl::fromLocalFile(filePath.toFSPathString());
}

void prepareNonNativeDialog(QFileDialog &dialog)
{
    const auto isValidSideBarPath = [](const FilePath &fp) {
        return !fp.needsDevice() || fp.hasFileAccess();
    };

    // Checking QFileDialog::itemDelegate() seems to be the only way to determine
    // whether the dialog is native or not.
    if (dialog.itemDelegate()) {
        FilePaths sideBarPaths;

        // Check existing urls, remove paths that need a device and are no longer valid.
        for (const QUrl &url : dialog.sidebarUrls()) {
            FilePath path = qUrlToFilePath(url);
            if (isValidSideBarPath(path))
                sideBarPaths.append(path);
        }

        // Add all device roots that are not already in the sidebar and valid.
        for (const FilePath &path : FSEngine::registeredDeviceRoots()) {
            if (!sideBarPaths.contains(path) && isValidSideBarPath(path))
                sideBarPaths.append(path);
        }

        dialog.setSidebarUrls(Utils::transform(sideBarPaths, filePathToQUrl));
        dialog.setIconProvider(Utils::FileIconProvider::iconProvider());
        dialog.setFilter(QDir::Hidden | dialog.filter());
    }
}

FilePaths getFilePaths(QWidget *parent,
                       const QString &caption,
                       const FilePath &dir,
                       const QString &filter,
                       QString *selectedFilter,
                       QFileDialog::Options options,
                       const QStringList &supportedSchemes,
                       const bool forceNonNativeDialog,
                       QFileDialog::FileMode fileMode,
                       QFileDialog::AcceptMode acceptMode)
{
    QFileDialog dialog(parent, caption, dir.toFSPathString(), filter);
    dialog.setFileMode(fileMode);

    if (forceNonNativeDialog)
        options.setFlag(QFileDialog::DontUseNativeDialog);

    dialog.setOptions(options);
    prepareNonNativeDialog(dialog);

    dialog.setSupportedSchemes(supportedSchemes);
    dialog.setAcceptMode(acceptMode);

    if (selectedFilter && !selectedFilter->isEmpty())
        dialog.selectNameFilter(*selectedFilter);
    if (dialog.exec() == QDialog::Accepted) {
        if (selectedFilter)
            *selectedFilter = dialog.selectedNameFilter();
        return Utils::transform(dialog.selectedUrls(), &qUrlToFilePath);
    }
    return {};
}

FilePath firstOrEmpty(const FilePaths &filePaths)
{
    return filePaths.isEmpty() ? FilePath() : filePaths.first();
}

bool FileUtils::hasNativeFileDialog()
{
    static std::optional<bool> hasNative;
    if (!hasNative.has_value()) {
        // Checking QFileDialog::itemDelegate() seems to be the only way to determine
        // whether the dialog is native or not.
        QFileDialog dialog;
        hasNative = dialog.itemDelegate() == nullptr;
    }

    return *hasNative;
}

FilePath FileUtils::getOpenFilePath(QWidget *parent,
                                    const QString &caption,
                                    const FilePath &dir,
                                    const QString &filter,
                                    QString *selectedFilter,
                                    QFileDialog::Options options,
                                    bool fromDeviceIfShiftIsPressed,
                                    bool forceNonNativeDialog)
{
    forceNonNativeDialog = forceNonNativeDialog || dir.needsDevice();
#ifdef QT_GUI_LIB
    if (fromDeviceIfShiftIsPressed && qApp->queryKeyboardModifiers() & Qt::ShiftModifier) {
        forceNonNativeDialog = true;
    }
#endif

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return firstOrEmpty(getFilePaths(dialogParent(parent),
                                     caption,
                                     dir,
                                     filter,
                                     selectedFilter,
                                     options,
                                     schemes,
                                     forceNonNativeDialog,
                                     QFileDialog::ExistingFile,
                                     QFileDialog::AcceptOpen));
}

FilePath FileUtils::getSaveFilePath(QWidget *parent,
                                    const QString &caption,
                                    const FilePath &dir,
                                    const QString &filter,
                                    QString *selectedFilter,
                                    QFileDialog::Options options,
                                    bool forceNonNativeDialog)
{
    forceNonNativeDialog = forceNonNativeDialog || dir.needsDevice();

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return firstOrEmpty(getFilePaths(dialogParent(parent),
                                     caption,
                                     dir,
                                     filter,
                                     selectedFilter,
                                     options,
                                     schemes,
                                     forceNonNativeDialog,
                                     QFileDialog::AnyFile,
                                     QFileDialog::AcceptSave));
}

FilePath FileUtils::getExistingDirectory(QWidget *parent,
                                         const QString &caption,
                                         const FilePath &dir,
                                         QFileDialog::Options options,
                                         bool fromDeviceIfShiftIsPressed,
                                         bool forceNonNativeDialog)
{
    forceNonNativeDialog = forceNonNativeDialog || dir.needsDevice();

#ifdef QT_GUI_LIB
    if (fromDeviceIfShiftIsPressed && qApp->queryKeyboardModifiers() & Qt::ShiftModifier) {
        forceNonNativeDialog = true;
    }
#endif

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return firstOrEmpty(getFilePaths(dialogParent(parent),
                                     caption,
                                     dir,
                                     {},
                                     nullptr,
                                     options,
                                     schemes,
                                     forceNonNativeDialog,
                                     QFileDialog::Directory,
                                     QFileDialog::AcceptOpen));
}

FilePaths FileUtils::getOpenFilePaths(QWidget *parent,
                                      const QString &caption,
                                      const FilePath &dir,
                                      const QString &filter,
                                      QString *selectedFilter,
                                      QFileDialog::Options options)
{
    bool forceNonNativeDialog = dir.needsDevice();

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return getFilePaths(dialogParent(parent),
                        caption,
                        dir,
                        filter,
                        selectedFilter,
                        options,
                        schemes,
                        forceNonNativeDialog,
                        QFileDialog::ExistingFiles,
                        QFileDialog::AcceptOpen);
}

#endif // QT_WIDGETS_LIB

FilePathInfo::FileFlags fileInfoFlagsfromStatMode(const QString &hexString, int modeBase)
{
    // Copied from stat.h
    enum st_mode {
        IFMT = 00170000,
        IFSOCK = 0140000,
        IFLNK = 0120000,
        IFREG = 0100000,
        IFBLK = 0060000,
        IFDIR = 0040000,
        IFCHR = 0020000,
        IFIFO = 0010000,
        ISUID = 0004000,
        ISGID = 0002000,
        ISVTX = 0001000,
        IRWXU = 00700,
        IRUSR = 00400,
        IWUSR = 00200,
        IXUSR = 00100,
        IRWXG = 00070,
        IRGRP = 00040,
        IWGRP = 00020,
        IXGRP = 00010,
        IRWXO = 00007,
        IROTH = 00004,
        IWOTH = 00002,
        IXOTH = 00001,
    };

    bool ok = false;
    uint mode = hexString.toUInt(&ok, modeBase);

    QTC_ASSERT(ok, return {});

    FilePathInfo::FileFlags result;

    if (mode & IRUSR) {
        result |= FilePathInfo::ReadOwnerPerm;
        result |= FilePathInfo::ReadUserPerm;
    }
    if (mode & IWUSR) {
        result |= FilePathInfo::WriteOwnerPerm;
        result |= FilePathInfo::WriteUserPerm;
    }
    if (mode & IXUSR) {
        result |= FilePathInfo::ExeOwnerPerm;
        result |= FilePathInfo::ExeUserPerm;
    }
    if (mode & IRGRP)
        result |= FilePathInfo::ReadGroupPerm;
    if (mode & IWGRP)
        result |= FilePathInfo::WriteGroupPerm;
    if (mode & IXGRP)
        result |= FilePathInfo::ExeGroupPerm;
    if (mode & IROTH)
        result |= FilePathInfo::ReadOtherPerm;
    if (mode & IWOTH)
        result |= FilePathInfo::WriteOtherPerm;
    if (mode & IXOTH)
        result |= FilePathInfo::ExeOtherPerm;

    if ((mode & IFMT) == IFLNK)
        result |= FilePathInfo::LinkType;
    if ((mode & IFMT) == IFREG)
        result |= FilePathInfo::FileType;
    if ((mode & IFMT) == IFDIR)
        result |= FilePathInfo::DirectoryType;
    if ((mode & IFMT) == IFBLK)
        result |= FilePathInfo::LocalDiskFlag;

    if (result != 0) // There is no Exist flag, but if anything was set before, it must exist.
        result |= FilePathInfo::ExistsFlag;

    return result;
}

FilePathInfo FileUtils::filePathInfoFromTriple(const QString &infos, int modeBase)
{
    const QStringList parts = infos.split(' ', Qt::SkipEmptyParts);
    if (parts.size() != 3)
        return {};

    FilePathInfo::FileFlags flags = fileInfoFlagsfromStatMode(parts[0], modeBase);

    const QDateTime dt = QDateTime::fromSecsSinceEpoch(parts[1].toLongLong(), Qt::UTC);
    qint64 size = parts[2].toLongLong();
    return {size, flags, dt};
}

bool FileUtils::copyRecursively(
    const FilePath &srcFilePath,
    const FilePath &tgtFilePath,
    QString *error,
    std::function<bool(const FilePath &, const FilePath &, QString *)> copyHelper)
{
    if (srcFilePath.isDir()) {
        if (!tgtFilePath.ensureWritableDir()) {
            if (error) {
                *error = Tr::tr("Failed to create directory \"%1\".")
                             .arg(tgtFilePath.toUserOutput());
            }
            return false;
        }
        const QDir sourceDir(srcFilePath.toString());
        const QStringList fileNames = sourceDir.entryList(
            QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QString &fileName : fileNames) {
            const FilePath newSrcFilePath = srcFilePath / fileName;
            const FilePath newTgtFilePath = tgtFilePath / fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath, error, copyHelper))
                return false;
        }
    } else {
        if (!copyHelper(srcFilePath, tgtFilePath, error))
            return false;
    }
    return true;
}

/*!
  Copies a file specified by \a srcFilePath to \a tgtFilePath only if \a srcFilePath is different
  (file contents and last modification time).

  Returns whether the operation succeeded.
*/

bool FileUtils::copyIfDifferent(const FilePath &srcFilePath, const FilePath &tgtFilePath)
{
    QTC_ASSERT(srcFilePath.exists(), qDebug() << srcFilePath.toUserOutput(); return false);
    QTC_ASSERT(srcFilePath.isSameDevice(tgtFilePath), return false);

    if (tgtFilePath.exists()) {
        const QDateTime srcModified = srcFilePath.lastModified();
        const QDateTime tgtModified = tgtFilePath.lastModified();
        if (srcModified == tgtModified) {
            // TODO: Create FilePath::hashFromContents() and compare hashes.
            const expected_str<QByteArray> srcContents = srcFilePath.fileContents();
            const expected_str<QByteArray> tgtContents = srcFilePath.fileContents();
            if (srcContents && srcContents == tgtContents)
                return true;
        }
        tgtFilePath.removeFile();
    }

    const expected_str<void> copyResult = srcFilePath.copyFile(tgtFilePath);

    // TODO forward error to caller instead of assert, since IO errors can always be expected
    QTC_ASSERT_EXPECTED(copyResult, return false);
    return true;
}

QString FileUtils::fileSystemFriendlyName(const QString &name)
{
    QString result = name;
    result.replace(QRegularExpression(QLatin1String("\\W")), QLatin1String("_"));
    result.replace(QRegularExpression(QLatin1String("_+")), QLatin1String("_")); // compact _
    result.remove(QRegularExpression(QLatin1String("^_*"))); // remove leading _
    result.remove(QRegularExpression(QLatin1String("_+$"))); // remove trailing _
    if (result.isEmpty())
        result = QLatin1String("unknown");
    return result;
}

int FileUtils::indexOfQmakeUnfriendly(const QString &name, int startpos)
{
    static const QRegularExpression checkRegExp(QLatin1String("[^a-zA-Z0-9_.-]"));
    return checkRegExp.match(name, startpos).capturedStart();
}

QString FileUtils::qmakeFriendlyName(const QString &name)
{
    QString result = name;

    // Remove characters that might trip up a build system (especially qmake):
    int pos = indexOfQmakeUnfriendly(result);
    while (pos >= 0) {
        result[pos] = QLatin1Char('_');
        pos = indexOfQmakeUnfriendly(result, pos);
    }
    return fileSystemFriendlyName(result);
}

bool FileUtils::makeWritable(const FilePath &path)
{
    return path.setPermissions(path.permissions() | QFile::WriteUser);
}

// makes sure that capitalization of directories is canonical on Windows and macOS.
// This mimics the logic in QDeclarative_isFileCaseCorrect
QString FileUtils::normalizedPathName(const QString &name)
{
#ifdef Q_OS_WIN
    const QString nativeSeparatorName(QDir::toNativeSeparators(name));
    const auto nameC = reinterpret_cast<LPCTSTR>(nativeSeparatorName.utf16()); // MinGW
    PIDLIST_ABSOLUTE file;
    HRESULT hr = SHParseDisplayName(nameC, NULL, &file, 0, NULL);
    if (FAILED(hr))
        return name;
    TCHAR buffer[MAX_PATH];
    const bool success = SHGetPathFromIDList(file, buffer);
    ILFree(file);
    return success ? QDir::fromNativeSeparators(QString::fromUtf16(reinterpret_cast<const ushort *>(buffer)))
                   : name;
#elif defined(Q_OS_MACOS)
    return Internal::normalizePathName(name);
#else // do not try to handle case-insensitive file systems on Linux
    return name;
#endif
}

FilePath FileUtils::commonPath(const FilePath &oldCommonPath, const FilePath &filePath)
{
    FilePath newCommonPath = oldCommonPath;
    while (!newCommonPath.isEmpty() && !filePath.isChildOf(newCommonPath))
        newCommonPath = newCommonPath.parentDir();
    return newCommonPath.canonicalPath();
}

FilePath FileUtils::homePath()
{
    return FilePath::fromUserInput(QDir::homePath());
}

FilePaths FileUtils::toFilePathList(const QStringList &paths)
{
    return transform(paths, &FilePath::fromString);
}

qint64 FileUtils::bytesAvailableFromDFOutput(const QByteArray &dfOutput)
{
    const auto lines = filtered(dfOutput.split('\n'),
                                [](const QByteArray &line) { return line.size() > 0; });

    QTC_ASSERT(lines.size() == 2, return -1);
    const auto headers = filtered(lines[0].split(' '),
                                  [](const QByteArray &field) { return field.size() > 0; });
    QTC_ASSERT(headers.size() >= 4, return -1);
    QTC_ASSERT(headers[3] == QByteArray("Available"), return -1);

    const auto fields = filtered(lines[1].split(' '),
                                 [](const QByteArray &field) { return field.size() > 0; });
    QTC_ASSERT(fields.size() >= 4, return -1);

    bool ok = false;
    const quint64 result = QString::fromUtf8(fields[3]).toULongLong(&ok);
    if (ok)
        return result;
    return -1;
}

FilePaths FileUtils::usefulExtraSearchPaths()
{
    if (HostOsInfo::isMacHost()) {
        return {"/opt/homebrew/bin"};
    } else if (HostOsInfo::isWindowsHost()) {
        const QString programData =
            qtcEnvironmentVariable("ProgramData",
                                   qtcEnvironmentVariable("ALLUSERSPROFILE", "C:/ProgramData"));
        return {FilePath::fromUserInput(programData) / "chocolatey/bin"};
    }

    return {};
}

} // namespace Utils
