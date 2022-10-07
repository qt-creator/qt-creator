// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "fileutils.h"
#include "savefile.h"

#include "algorithm.h"
#include "commandline.h"
#include "qtcassert.h"
#include "hostosinfo.h"

#include "fsengine/fileiconprovider.h"
#include "fsengine/fsengine.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QOperatingSystemVersion>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QTextStream>
#include <QXmlStreamWriter>

#include <qplatformdefs.h>

#ifdef QT_GUI_LIB
#include <QMessageBox>
#include <QRegularExpression>
#include <QGuiApplication>
#endif

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#include <shlobj.h>
#endif

#ifdef Q_OS_OSX
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

    if (filePath.needsDevice()) {
        const std::optional<QByteArray> contents = filePath.fileContents();
        if (!contents) {
            m_errorString = tr("Cannot read %1").arg(filePath.toUserOutput());
            return false;
        }
        m_data = *contents;
        return true;
    }

    QFile file(filePath.toString());
    if (!file.open(QIODevice::ReadOnly | mode)) {
        m_errorString = tr("Cannot open %1 for reading: %2").arg(
                filePath.toUserOutput(), file.errorString());
        return false;
    }
    m_data = file.readAll();
    if (file.error() != QFile::NoError) {
        m_errorString = tr("Cannot read %1: %2").arg(
                filePath.toUserOutput(), file.errorString());
        return false;
    }
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
        QMessageBox::critical(parent, tr("File Error"), m_errorString);
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
    QMessageBox::critical(parent, tr("File Error"), errorString());
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
    return setResult(m_file->write(bytes) == bytes.count());
}

bool FileSaverBase::setResult(bool ok)
{
    if (!ok && !m_hasError) {
        if (!m_file->errorString().isEmpty()) {
            m_errorString = tr("Cannot write file %1: %2")
                                .arg(m_filePath.toUserOutput(), m_file->errorString());
        } else {
            m_errorString = tr("Cannot write file %1. Disk full?").arg(m_filePath.toUserOutput());
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
            m_errorString = tr("%1: Is a reserved filename on Windows. Cannot save.")
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
    } else if (mode & (QIODevice::ReadOnly | QIODevice::Append)) {
        m_file.reset(new QFile{filePath.path()});
        m_isSafe = false;
    } else {
        m_file.reset(new SaveFile{filePath.path()});
        m_isSafe = true;
    }
    if (!m_file->open(QIODevice::WriteOnly | mode)) {
        QString err = filePath.exists() ?
                tr("Cannot overwrite file %1: %2") : tr("Cannot create file %1: %2");
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
        const bool res = m_filePath.writeFileContents(data);
        m_file->remove();
        m_file.reset();
        return res;
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
        m_errorString = tr("Cannot create temporary file in %1: %2").arg(
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

/*! \class Utils::FileUtils

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
                QCoreApplication::translate("Utils::FileUtils", "Overwrite File?"),
                QCoreApplication::translate("Utils::FileUtils", "Overwrite existing file \"%1\"?")
                    .arg(dest.toUserOutput()),
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
                *error = QCoreApplication::translate("Utils::FileUtils",
                                                     "Could not copy file \"%1\" to \"%2\".")
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

// Copied from qfilesystemengine_win.cpp
#ifdef Q_OS_WIN

// File ID for Windows up to version 7.
static inline QByteArray fileIdWin7(HANDLE handle)
{
    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(handle, &info)) {
        char buffer[sizeof "01234567:0123456701234567\0"];
        qsnprintf(buffer, sizeof(buffer), "%lx:%08lx%08lx",
                  info.dwVolumeSerialNumber,
                  info.nFileIndexHigh,
                  info.nFileIndexLow);
        return QByteArray(buffer);
    }
    return QByteArray();
}

// File ID for Windows starting from version 8.
static QByteArray fileIdWin8(HANDLE handle)
{
    QByteArray result;
    FILE_ID_INFO infoEx;
    if (GetFileInformationByHandleEx(handle,
                                     static_cast<FILE_INFO_BY_HANDLE_CLASS>(18), // FileIdInfo in Windows 8
                                     &infoEx, sizeof(FILE_ID_INFO))) {
        result = QByteArray::number(infoEx.VolumeSerialNumber, 16);
        result += ':';
        // Note: MinGW-64's definition of FILE_ID_128 differs from the MSVC one.
        result += QByteArray(reinterpret_cast<const char *>(&infoEx.FileId), int(sizeof(infoEx.FileId))).toHex();
    }
    return result;
}

static QByteArray fileIdWin(HANDLE fHandle)
{
    return QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8 ?
                fileIdWin8(HANDLE(fHandle)) : fileIdWin7(HANDLE(fHandle));
}
#endif

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

QByteArray FileUtils::fileId(const FilePath &fileName)
{
    QByteArray result;

#ifdef Q_OS_WIN
    const HANDLE handle =
            CreateFile((wchar_t*)fileName.toUserOutput().utf16(), 0,
                       FILE_SHARE_READ, NULL, OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        result = fileIdWin(handle);
        CloseHandle(handle);
    }
#else // Copied from qfilesystemengine_unix.cpp
    if (Q_UNLIKELY(fileName.isEmpty()))
        return result;

    QT_STATBUF statResult;
    if (QT_STAT(fileName.toString().toLocal8Bit().constData(), &statResult))
        return result;
    result = QByteArray::number(quint64(statResult.st_dev), 16);
    result += ':';
    result += QByteArray::number(quint64(statResult.st_ino));
#endif
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

FilePath FileUtils::getOpenFilePath(QWidget *parent,
                                    const QString &caption,
                                    const FilePath &dir,
                                    const QString &filter,
                                    QString *selectedFilter,
                                    QFileDialog::Options options,
                                    bool fromDeviceIfShiftIsPressed)
{
#ifdef QT_GUI_LIB
    if (fromDeviceIfShiftIsPressed && qApp->queryKeyboardModifiers() & Qt::ShiftModifier) {
        return getOpenFilePathFromDevice(parent, caption, dir, filter, selectedFilter, options);
    }
#endif

    const QString result = QFileDialog::getOpenFileName(dialogParent(parent),
                                                        caption,
                                                        dir.toString(),
                                                        filter,
                                                        selectedFilter,
                                                        options);
    return FilePath::fromString(result);
}

FilePath FileUtils::getSaveFilePath(QWidget *parent,
                                    const QString &caption,
                                    const FilePath &dir,
                                    const QString &filter,
                                    QString *selectedFilter,
                                    QFileDialog::Options options)
{
    const QString result = QFileDialog::getSaveFileName(dialogParent(parent),
                                                        caption,
                                                        dir.toString(),
                                                        filter,
                                                        selectedFilter,
                                                        options);
    return FilePath::fromString(result);
}

FilePath FileUtils::getExistingDirectory(QWidget *parent,
                                         const QString &caption,
                                         const FilePath &dir,
                                         QFileDialog::Options options)
{
    const QString result = QFileDialog::getExistingDirectory(dialogParent(parent),
                                                             caption,
                                                             dir.toString(),
                                                             options);
    return FilePath::fromString(result);
}

FilePaths FileUtils::getOpenFilePaths(QWidget *parent,
                                      const QString &caption,
                                      const FilePath &dir,
                                      const QString &filter,
                                      QString *selectedFilter,
                                      QFileDialog::Options options)
{
    const QStringList result = QFileDialog::getOpenFileNames(dialogParent(parent),
                                                             caption,
                                                             dir.toString(),
                                                             filter,
                                                             selectedFilter,
                                                             options);
    return FileUtils::toFilePathList(result);
}

FilePath FileUtils::getOpenFilePathFromDevice(QWidget *parent,
                                              const QString &caption,
                                              const FilePath &dir,
                                              const QString &filter,
                                              QString *selectedFilter,
                                              QFileDialog::Options options)
{
    QFileDialog dialog(parent);
    dialog.setOptions(options | QFileDialog::DontUseNativeDialog);
    dialog.setWindowTitle(caption);
    dialog.setDirectory(dir.toString());
    dialog.setNameFilter(filter);

    QList<QUrl> sideBarUrls = Utils::transform(Utils::filtered(FSEngine::registeredDeviceRoots(),
                                                               [](const auto &filePath) {
                                                                   return filePath.exists();
                                                               }),
                                               [](const auto &filePath) {
                                                   return QUrl::fromLocalFile(
                                                       filePath.toFSPathString());
                                               });
    dialog.setSidebarUrls(sideBarUrls);
    dialog.setFileMode(QFileDialog::AnyFile);

    dialog.setIconProvider(Utils::FileIconProvider::iconProvider());

    if (dialog.exec()) {
        FilePaths filePaths = Utils::transform(dialog.selectedFiles(), [](const auto &path) {
            return FilePath::fromString(path);
        });

        if (selectedFilter) {
            *selectedFilter = dialog.selectedNameFilter();
        }

        return filePaths.first();
    }

    return {};
}

#endif // QT_WIDGETS_LIB

// Used on 'ls' output on unix-like systems.
void FileUtils::iterateLsOutput(const FilePath &base,
                                const QStringList &entries,
                                const FileFilter &filter,
                                const std::function<bool (const FilePath &)> &callBack)
{
    const QList<QRegularExpression> nameRegexps =
            transform(filter.nameFilters, [](const QString &filter) {
        QRegularExpression re;
        re.setPattern(QRegularExpression::wildcardToRegularExpression(filter));
        QTC_CHECK(re.isValid());
        return re;
    });

    const auto nameMatches = [&nameRegexps](const QString &fileName) {
        for (const QRegularExpression &re : nameRegexps) {
            const QRegularExpressionMatch match = re.match(fileName);
            if (match.hasMatch())
                return true;
        }
        return nameRegexps.isEmpty();
    };

    // FIXME: Handle filters. For now bark on unsupported options.
    QTC_CHECK(filter.fileFilters == QDir::NoFilter);

    for (const QString &entry : entries) {
        if (!nameMatches(entry))
            continue;
        if (!callBack(base.pathAppended(entry)))
            break;
    }
}

// returns whether 'find' could be used.
static bool iterateWithFind(const FilePath &filePath,
                            const FileFilter &filter,
                            const std::function<RunResult(const CommandLine &)> &runInShell,
                            const std::function<bool(const FilePath &)> &callBack)
{
    QTC_CHECK(filePath.isAbsolutePath());
    QStringList arguments{filePath.path()};
    arguments << filter.asFindArguments();

    const RunResult result = runInShell({"find", arguments});
    if (!result.stdErr.isEmpty()) {
        // missing find, unknown option e.g. "find: unknown predicate `-L'\n"
        // qDebug() << "find error: " << result.stdErr;
        return false;
    }

    const QString out = QString::fromUtf8(result.stdOut);
    const QStringList entries = out.split("\n", Qt::SkipEmptyParts);
    for (const QString &entry : entries) {
        const FilePath fp = FilePath::fromString(entry);
        // Call back returning 'false' indicates a request to abort iteration.
        if (!callBack(fp.onDevice(filePath)))
            break;
    }
    return true;
}

void FileUtils::iterateUnixDirectory(const FilePath &filePath,
                                     const FileFilter &filter,
                                     bool *useFind,
                                     const std::function<RunResult (const CommandLine &)> &runInShell,
                                     const std::function<bool(const FilePath &)> &callBack)
{
    QTC_ASSERT(callBack, return);

    // We try to use 'find' first, because that can filter better directly.
    // Unfortunately, it's not installed on all devices by default.
    if (useFind && *useFind) {
        if (iterateWithFind(filePath, filter, runInShell, callBack))
            return;
        *useFind = false; // remember the failure for the next time and use the 'ls' fallback below.
    }

    // if we do not have find - use ls as fallback
    // FIXME: Recursion into subdirectories not implemented!
    const RunResult result = runInShell({"ls", {"-1", "-b", "--", filePath.path()}});
    const QStringList entries = QString::fromUtf8(result.stdOut).split('\n', Qt::SkipEmptyParts);
    FileUtils::iterateLsOutput(filePath, entries, filter, callBack);
}

/*!
  Copies the directory specified by \a srcFilePath recursively to \a tgtFilePath. \a tgtFilePath will contain
  the target directory, which will be created. Example usage:

  \code
    QString error;
    bool ok = Utils::FileUtils::copyRecursively("/foo/bar", "/foo/baz", &error);
    if (!ok)
      qDebug() << error;
  \endcode

  This will copy the contents of /foo/bar into to the baz directory under /foo, which will be created in the process.

  \note The \a error parameter is optional.

  Returns whether the operation succeeded.
*/

bool FileUtils::copyRecursively(const FilePath &srcFilePath, const FilePath &tgtFilePath, QString *error)
{
    return copyRecursively(
        srcFilePath, tgtFilePath, error, [](const FilePath &src, const FilePath &dest, QString *error) {
            if (!src.copyFile(dest)) {
                if (error) {
                    *error = QCoreApplication::translate("Utils::FileUtils",
                                                         "Could not copy file \"%1\" to \"%2\".")
                                 .arg(src.toUserOutput(), dest.toUserOutput());
                }
                return false;
            }
            return true;
        });
}

/*!
  Copies a file specified by \a srcFilePath to \a tgtFilePath only if \a srcFilePath is different
  (file contents and last modification time).

  Returns whether the operation succeeded.
*/

bool FileUtils::copyIfDifferent(const FilePath &srcFilePath, const FilePath &tgtFilePath)
{
    QTC_ASSERT(srcFilePath.exists(), return false);
    QTC_ASSERT(srcFilePath.scheme() == tgtFilePath.scheme(), return false);
    QTC_ASSERT(srcFilePath.host() == tgtFilePath.host(), return false);

    if (tgtFilePath.exists()) {
        const QDateTime srcModified = srcFilePath.lastModified();
        const QDateTime tgtModified = tgtFilePath.lastModified();
        if (srcModified == tgtModified) {
            const std::optional<QByteArray> srcContents = srcFilePath.fileContents();
            const std::optional<QByteArray> tgtContents = srcFilePath.fileContents();
            if (srcContents == tgtContents)
                return true;
        }
        tgtFilePath.removeFile();
    }

    return srcFilePath.copyFile(tgtFilePath);
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

bool isRelativePathHelper(const QString &path, OsType osType)
{
    if (path.startsWith('/'))
        return false;
    if (osType == OsType::OsTypeWindows) {
        if (path.startsWith('\\'))
            return false;
        // Unlike QFileInfo, this won't accept a relative path with a drive letter.
        // Such paths result in a royal mess anyway ...
        if (path.length() >= 3 && path.at(1) == ':' && path.at(0).isLetter()
                && (path.at(2) == '/' || path.at(2) == '\\'))
            return false;
    }
    return true;
}

bool FileUtils::isRelativePath(const QString &path)
{
    return isRelativePathHelper(path, HostOsInfo::hostOs());
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
    return FilePath::fromString(doCleanPath(QDir::homePath()));
}

FilePaths FileUtils::toFilePathList(const QStringList &paths) {
    return transform(paths, [](const QString &path) { return FilePath::fromString(path); });
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

} // namespace Utils
