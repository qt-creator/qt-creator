// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileutils.h"
#include "savefile.h"

#include "algorithm.h"
#include "environment.h"
#include "qtcassert.h"
#include "utilstr.h"

#include "fsengine/fileiconprovider.h"
#include "fsengine/fsengine.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QTextStream>
#include <QXmlStreamWriter>

#include <qplatformdefs.h>

#ifdef QT_GUI_LIB
#include "guiutils.h"

#include <QMessageBox>
#include <QGuiApplication>
#endif

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#include <shlobj.h>
#include <winioctl.h>
#endif

#ifdef Q_OS_MACOS
#include "fileutils_mac.h"
#endif

namespace Utils {

// FileSaver

FileSaverBase::FileSaverBase()
    : m_result(ResultOk)
{}

FileSaverBase::~FileSaverBase() = default;

Result<> FileSaverBase::finalize()
{
    m_file->close();
    setResult(m_file->error() == QFile::NoError);
    m_file.reset();
    return m_result;
}

bool FileSaverBase::write(const QByteArrayView bytes)
{
    if (!m_result)
        return false;
    return setResult(m_file->write(bytes.data(), bytes.size()) == bytes.size());
}

bool FileSaverBase::setResult(bool ok)
{
    if (!ok && m_result) {
        if (!m_file->errorString().isEmpty()) {
            m_result = ResultError(Tr::tr("Cannot write file %1: %2")
                                     .arg(m_filePath.toUserOutput(), m_file->errorString()));
        } else {
            m_result = ResultError(Tr::tr("Cannot write file %1. Disk full?")
                                     .arg(m_filePath.toUserOutput()));
        }
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

static bool saveFileSupportedFileSystem(const FilePath &path)
{
#ifdef Q_OS_WIN
    const HANDLE handle = CreateFile((wchar_t *) path.toUserOutput().utf16(),
                                     0,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,
                                     NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        FILESYSTEM_STATISTICS stats;
        DWORD bytesReturned;
        bool success = DeviceIoControl(
            handle,
            FSCTL_FILESYSTEM_GET_STATISTICS,
            NULL,
            0,
            &stats,
            sizeof(stats),
            &bytesReturned,
            NULL);
        CloseHandle(handle);
        if (success || GetLastError() == ERROR_MORE_DATA) {
            return stats.FileSystemType != FILESYSTEM_STATISTICS_TYPE_FAT
                   && stats.FileSystemType != FILESYSTEM_STATISTICS_TYPE_EXFAT;
        }
    }
#else
    Q_UNUSED(path)
#endif
    return true;
}

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
            m_result = ResultError(Tr::tr("%1: Is a reserved filename on Windows. Cannot save.")
                                     .arg(filePath.toUserOutput()));
            return;
        }
    }

    const bool readOnlyOrAppend = mode & (QIODevice::ReadOnly | QIODevice::Append);
    m_isSafe = !readOnlyOrAppend && !filePath.hasHardLinks()
               && !qtcEnvironmentVariableIsSet("QTC_DISABLE_ATOMICSAVE")
               && saveFileSupportedFileSystem(filePath);
    if (m_isSafe)
        m_file.reset(new SaveFile(filePath));
    else
        m_file.reset(new QFile{filePath.toFSPathString()});

    if (!m_file->open(QIODevice::WriteOnly | mode)) {
        QString err = filePath.exists() ?
                Tr::tr("Cannot overwrite file %1: %2") : Tr::tr("Cannot create file %1: %2");
        m_result = ResultError(err.arg(filePath.toUserOutput(), m_file->errorString()));
    }
}

Result<> FileSaver::finalize()
{
    if (!m_isSafe)
        return FileSaverBase::finalize();

    auto sf = static_cast<SaveFile *>(m_file.get());
    if (!m_result) {
        if (sf->isOpen())
            sf->rollback();
    } else {
        setResult(sf->commit());
    }
    m_file.reset();
    return m_result;
}

TempFileSaver::TempFileSaver(const QString &templ)
{
    initFromString(templ);
}

void TempFileSaver::initFromString(const QString &templ)
{
    m_file.reset(new QTemporaryFile{});
    auto tempFile = static_cast<QTemporaryFile *>(m_file.get());
    if (!templ.isEmpty())
        tempFile->setFileTemplate(templ);
    tempFile->setAutoRemove(false);
    if (!tempFile->open()) {
        m_result = ResultError(Tr::tr("Cannot create temporary file in %1: %2").arg(
                QDir::toNativeSeparators(QFileInfo(tempFile->fileTemplate()).absolutePath()),
                tempFile->errorString()));
    }
    m_filePath = FilePath::fromString(tempFile->fileName());
}

TempFileSaver::TempFileSaver(const FilePath &templ)
{
    if (templ.isEmpty() || templ.isLocal()) {
        initFromString(templ.path());
    } else {
        Result<FilePath> result = templ.createTempFile();
        if (!result) {
            m_result = ResultError(Tr::tr("Cannot create temporary file %1: %2")
                                .arg(templ.toUserOutput(), result.error()));
            return;
        }

        m_file.reset(new QFile(result->toFSPathString()));
        if (!m_file->open(QIODevice::WriteOnly)) {
            m_result = ResultError(Tr::tr("Cannot create temporary file %1: %2")
                                .arg(result->toUserOutput(), m_file->errorString()));
        }
        m_filePath = *result;
    }
}

TempFileSaver::~TempFileSaver()
{
    m_file.reset();
    if (m_autoRemove)
        m_filePath.removeFile();
}

/*!
    \namespace Utils::FileUtils
    \inmodule QtCreator

  \brief The FileUtils namespace contains file and directory related convenience
  functions.

*/
namespace FileUtils {

#ifdef QT_GUI_LIB
CopyAskingForOverwrite::CopyAskingForOverwrite(const std::function<void (FilePath)> &postOperation)
    : m_postOperation(postOperation)
{}

CopyHelper CopyAskingForOverwrite::operator()()
{
    CopyHelper helperFunction = [this](const FilePath &src, const FilePath &dest, QString *error) {
        bool copyFile = true;
        if (dest.exists()) {
            if (m_skipAll)
                copyFile = false;
            else if (!m_overwriteAll) {
                const int res = QMessageBox::question(
                    dialogParent(),
                    Tr::tr("Overwrite File?"),
                    Tr::tr("Overwrite existing file \"%1\"?").arg(dest.toUserOutput()),
                    QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No
                            | QMessageBox::NoToAll | QMessageBox::Cancel);
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
            } else {
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
    };
    return helperFunction;
}

FilePaths CopyAskingForOverwrite::files() const
{
    return m_files;
}
#endif // QT_GUI_LIB

FilePath commonPath(const FilePaths &paths)
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

#ifdef QT_WIDGETS_LIB

QString fetchQrc(const QString &fileName)
{
    QTC_ASSERT(fileName.startsWith(':'), return QString());
    QFile file(fileName);
    bool ok = file.open(QIODevice::ReadOnly);
    QTC_ASSERT(ok, qWarning() << fileName << "not there!"; return QString());
    return QString::fromUtf8(file.readAll());
}

static QUrl filePathToQUrl(const FilePath &filePath)
{
    return QUrl::fromLocalFile(filePath.toFSPathString());
}

static void prepareNonNativeDialog(QFileDialog &dialog)
{
    const auto isValidSideBarPath = [](const FilePath &fp) {
        return fp.isLocal() || fp.hasFileAccess();
    };

    // Checking QFileDialog::itemDelegate() seems to be the only way to determine
    // whether the dialog is native or not.
    if (dialog.itemDelegate()) {
        FilePaths sideBarPaths;

        // Check existing urls, remove paths that need a device and are no longer valid.
        for (const QUrl &url : dialog.sidebarUrls()) {
            FilePath path = FilePath::fromUrl(url);
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

FilePaths getFilePaths(const QString &caption,
                       const FilePath &dir,
                       const QString &filter,
                       QString *selectedFilter,
                       QFileDialog::Options options,
                       const QStringList &supportedSchemes,
                       const bool forceNonNativeDialog,
                       QFileDialog::FileMode fileMode,
                       QFileDialog::AcceptMode acceptMode)
{
    QFileDialog dialog(dialogParent(), caption, dir.toFSPathString(), filter);
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
        return Utils::transform(dialog.selectedUrls(), &FilePath::fromUrl);
    }
    return {};
}

FilePath firstOrEmpty(const FilePaths &filePaths)
{
    return filePaths.isEmpty() ? FilePath() : filePaths.first();
}

bool hasNativeFileDialog()
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

FilePath getOpenFilePath(const QString &caption,
                         const FilePath &dir,
                         const QString &filter,
                         QString *selectedFilter,
                         QFileDialog::Options options,
                         bool fromDeviceIfShiftIsPressed,
                         bool forceNonNativeDialog)
{
    forceNonNativeDialog = forceNonNativeDialog || !dir.isLocal();
#ifdef QT_GUI_LIB
    if (fromDeviceIfShiftIsPressed && qApp->queryKeyboardModifiers() & Qt::ShiftModifier) {
        forceNonNativeDialog = true;
    }
#endif

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return firstOrEmpty(getFilePaths(caption,
                                     dir,
                                     filter,
                                     selectedFilter,
                                     options,
                                     schemes,
                                     forceNonNativeDialog,
                                     QFileDialog::ExistingFile,
                                     QFileDialog::AcceptOpen));
}

FilePath getSaveFilePath(const QString &caption,
                         const FilePath &dir,
                         const QString &filter,
                         QString *selectedFilter,
                         QFileDialog::Options options,
                         bool forceNonNativeDialog)
{
    forceNonNativeDialog = forceNonNativeDialog || !dir.isLocal();

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return firstOrEmpty(getFilePaths(caption,
                                     dir,
                                     filter,
                                     selectedFilter,
                                     options,
                                     schemes,
                                     forceNonNativeDialog,
                                     QFileDialog::AnyFile,
                                     QFileDialog::AcceptSave));
}

FilePath getExistingDirectory(const QString &caption,
                              const FilePath &dir,
                              QFileDialog::Options options,
                              bool fromDeviceIfShiftIsPressed,
                              bool forceNonNativeDialog)
{
    forceNonNativeDialog = forceNonNativeDialog || !dir.isLocal();

#ifdef QT_GUI_LIB
    if (fromDeviceIfShiftIsPressed && qApp->queryKeyboardModifiers() & Qt::ShiftModifier) {
        forceNonNativeDialog = true;
    }
#endif

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return firstOrEmpty(getFilePaths(caption,
                                     dir,
                                     {},
                                     nullptr,
                                     options,
                                     schemes,
                                     forceNonNativeDialog,
                                     QFileDialog::Directory,
                                     QFileDialog::AcceptOpen));
}

FilePaths getOpenFilePaths(const QString &caption,
                           const FilePath &dir,
                           const QString &filter,
                           QString *selectedFilter,
                           QFileDialog::Options options)
{
    bool forceNonNativeDialog = !dir.isLocal();

    const QStringList schemes = QStringList(QStringLiteral("file"));
    return getFilePaths(caption,
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

FilePathInfo filePathInfoFromTriple(const QString &infos, int modeBase)
{
    const QStringList parts = infos.split(' ', Qt::SkipEmptyParts);
    if (parts.size() != 3)
        return {};

    FilePathInfo::FileFlags flags = fileInfoFlagsfromStatMode(parts[0], modeBase);

    const QDateTime dt = QDateTime::fromSecsSinceEpoch(parts[1].toLongLong(), Qt::UTC);
    qint64 size = parts[2].toLongLong();
    return {size, flags, dt};
}

bool copyRecursively(const FilePath &srcFilePath,
                     const FilePath &tgtFilePath,
                     QString *error,
                     CopyHelper copyHelper)
{
    if (srcFilePath.isDir()) {
        if (!tgtFilePath.ensureWritableDir()) {
            if (error) {
                *error = Tr::tr("Failed to create directory \"%1\".")
                             .arg(tgtFilePath.toUserOutput());
            }
            return false;
        }
        const QDir sourceDir(srcFilePath.toFSPathString());
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

Result<> copyIfDifferent(const FilePath &srcFilePath, const FilePath &tgtFilePath)
{
    if (!srcFilePath.exists())
        return ResultError(Tr::tr("File %1 does not exist.").arg(srcFilePath.toUserOutput()));

    if (!srcFilePath.isLocal() || !tgtFilePath.isLocal())
        return srcFilePath.copyFile(tgtFilePath);

    if (tgtFilePath.exists()) {
        const QDateTime srcModified = srcFilePath.lastModified();
        const QDateTime tgtModified = tgtFilePath.lastModified();
        if (srcModified == tgtModified) {
            // TODO: Create FilePath::hashFromContents() and compare hashes.
            const Result<QByteArray> srcContents = srcFilePath.fileContents();
            const Result<QByteArray> tgtContents = tgtFilePath.fileContents();
            if (srcContents && srcContents == tgtContents)
                return ResultOk;
        }

        if (Result<> res = tgtFilePath.removeFile(); !res)
            return res;
    }

    return srcFilePath.copyFile(tgtFilePath);
}

QString fileSystemFriendlyName(const QString &name)
{
    QString result = name;
    static const QRegularExpression nonWordEx("\\W");
    result.replace(nonWordEx, QLatin1String("_"));
    static const QRegularExpression subsequentUnderscoreEx("_+");
    result.replace(subsequentUnderscoreEx, QLatin1String("_"));
    static const QRegularExpression leadingUnderscoreEx("^_*");
    result.remove(leadingUnderscoreEx);
    static const QRegularExpression trailingUnderscoreEx("_+$");
    result.remove(trailingUnderscoreEx);
    if (result.isEmpty())
        result = QLatin1String("unknown");
    return result;
}

int indexOfQmakeUnfriendly(const QString &name, int startpos)
{
    static const QRegularExpression checkRegExp(QLatin1String("[^a-zA-Z0-9_.-]"));
    return checkRegExp.match(name, startpos).capturedStart();
}

QString qmakeFriendlyName(const QString &name)
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

// makes sure that capitalization of directories is canonical on Windows and macOS.
// This mimics the logic in QDeclarative_isFileCaseCorrect
QString normalizedPathName(const QString &name)
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
    return success ? QDir::fromNativeSeparators(QString::fromUtf16(reinterpret_cast<const char16_t *>(buffer)))
                   : name;
#elif defined(Q_OS_MACOS)
    return Internal::normalizePathName(name);
#else // do not try to handle case-insensitive file systems on Linux
    return name;
#endif
}

FilePath commonPath(const FilePath &oldCommonPath, const FilePath &filePath)
{
    FilePath newCommonPath = oldCommonPath;
    while (!newCommonPath.isEmpty() && !filePath.isChildOf(newCommonPath))
        newCommonPath = newCommonPath.parentDir();
    return newCommonPath.canonicalPath();
}

FilePath homePath()
{
    return FilePath::fromUserInput(QDir::homePath());
}

Result<FilePath> scratchBufferFilePath(const QString &pattern)
{
    QString tmp = pattern;
    QFileInfo fi(tmp);
    if (!fi.isAbsolute()) {
        QString tempPattern = QDir::tempPath();
        if (!tempPattern.endsWith(QLatin1Char('/')))
            tempPattern += QLatin1Char('/');
        tmp = tempPattern + tmp;
    }

    QTemporaryFile file(tmp);
    file.setAutoRemove(false);
    if (!file.open()) {
        return ResultError(Tr::tr("Failed to set up scratch buffer in \"%1\".")
                                   .arg(FilePath::fromString(tmp).parentDir().toUserOutput()));
    }
    file.close();
    return FilePath::fromString(file.fileName());
}

FilePaths toFilePathList(const QStringList &paths)
{
    return transform(paths, &FilePath::fromString);
}

qint64 bytesAvailableFromDFOutput(const QByteArray &dfOutput)
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

FilePaths usefulExtraSearchPaths()
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

void showError(const QString &errorMessage)
{
    QMessageBox::critical(dialogParent(), Tr::tr("File Error"), errorMessage);
}

} // namespace FileUtils

#ifdef Q_OS_WIN
template <>
void withNtfsPermissions(const std::function<void()> &task)
{
    qt_ntfs_permission_lookup++;
    task();
    qt_ntfs_permission_lookup--;
}
#endif

} // namespace Utils
