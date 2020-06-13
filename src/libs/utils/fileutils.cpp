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

#include "fileutils.h"
#include "savefile.h"

#include "algorithm.h"
#include "qtcassert.h"
#include "qtcprocess.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QOperatingSystemVersion>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <qplatformdefs.h>

#ifdef QT_GUI_LIB
#include <QMessageBox>
#endif

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <shlobj.h>
#endif

#ifdef Q_OS_OSX
#include "fileutils_mac.h"
#endif

QT_BEGIN_NAMESPACE
QDebug operator<<(QDebug dbg, const Utils::FilePath &c)
{
    return dbg << c.toString();
}

QT_END_NAMESPACE

namespace Utils {

/*! \class Utils::CommandLine

 \brief The CommandLine class represents a command line of a QProcess
 or similar utility.

*/

CommandLine::CommandLine() = default;

CommandLine::CommandLine(const QString &executable)
    : m_executable(FilePath::fromString(executable))
{}

CommandLine::CommandLine(const FilePath &executable)
    : m_executable(executable)
{}

CommandLine::CommandLine(const QString &exe, const QStringList &args)
    : CommandLine(FilePath::fromString(exe), args)
{}

CommandLine::CommandLine(const FilePath &exe, const QStringList &args)
    : m_executable(exe)
{
    addArgs(args);
}

CommandLine::CommandLine(const FilePath &exe, const QString &args, RawType)
    : m_executable(exe)
{
    addArgs(args, Raw);
}

void CommandLine::addArg(const QString &arg, OsType osType)
{
    QtcProcess::addArg(&m_arguments, arg, osType);
}

void CommandLine::addArgs(const QStringList &inArgs, OsType osType)
{
    for (const QString &arg : inArgs)
        addArg(arg, osType);
}

void CommandLine::addArgs(const QString &inArgs, RawType)
{
    QtcProcess::addArgs(&m_arguments, inArgs);
}

QString CommandLine::toUserOutput() const
{
    return m_executable.toUserOutput() + ' ' + m_arguments;
}

QStringList CommandLine::splitArguments(OsType osType) const
{
    return QtcProcess::splitArgs(m_arguments, osType);
}

/*! \class Utils::FileUtils

  \brief The FileUtils class contains file and directory related convenience
  functions.

*/

/*!
  Removes the directory \a filePath and its subdirectories recursively.

  \note The \a error parameter is optional.

  Returns whether the operation succeeded.
*/
bool FileUtils::removeRecursively(const FilePath &filePath, QString *error)
{
    QFileInfo fileInfo = filePath.toFileInfo();
    if (!fileInfo.exists() && !fileInfo.isSymLink())
        return true;
    QFile::setPermissions(filePath.toString(), fileInfo.permissions() | QFile::WriteUser);
    if (fileInfo.isDir()) {
        QDir dir(filePath.toString());
        dir.setPath(dir.canonicalPath());
        if (dir.isRoot()) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils",
                    "Refusing to remove root directory.");
            }
            return false;
        }
        if (dir.path() == QDir::home().canonicalPath()) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils",
                    "Refusing to remove your home directory.");
            }
            return false;
        }

        const QStringList fileNames = dir.entryList(
                    QDir::Files | QDir::Hidden | QDir::System | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &fileName : fileNames) {
            if (!removeRecursively(filePath / fileName, error))
                return false;
        }
        if (!QDir::root().rmdir(dir.path())) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils", "Failed to remove directory \"%1\".")
                        .arg(filePath.toUserOutput());
            }
            return false;
        }
    } else {
        if (!QFile::remove(filePath.toString())) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils", "Failed to remove file \"%1\".")
                        .arg(filePath.toUserOutput());
            }
            return false;
        }
    }
    return true;
}

/*!
  Copies the directory specified by \a srcFilePath recursively to \a tgtFilePath. \a tgtFilePath will contain
  the target directory, which will be created. Example usage:

  \code
    QString error;
    book ok = Utils::FileUtils::copyRecursively("/foo/bar", "/foo/baz", &error);
    if (!ok)
      qDebug() << error;
  \endcode

  This will copy the contents of /foo/bar into to the baz directory under /foo, which will be created in the process.

  \note The \a error parameter is optional.

  Returns whether the operation succeeded.
*/
bool FileUtils::copyRecursively(const FilePath &srcFilePath, const FilePath &tgtFilePath,
                                QString *error, const std::function<bool (QFileInfo, QFileInfo, QString *)> &copyHelper)
{
    QFileInfo srcFileInfo = srcFilePath.toFileInfo();
    if (srcFileInfo.isDir()) {
        if (!tgtFilePath.exists()) {
            QDir targetDir(tgtFilePath.toString());
            targetDir.cdUp();
            if (!targetDir.mkdir(tgtFilePath.fileName())) {
                if (error) {
                    *error = QCoreApplication::translate("Utils::FileUtils", "Failed to create directory \"%1\".")
                            .arg(tgtFilePath.toUserOutput());
                }
                return false;
            }
        }
        QDir sourceDir(srcFilePath.toString());
        const QStringList fileNames = sourceDir.entryList(
                    QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QString &fileName : fileNames) {
            const FilePath newSrcFilePath = srcFilePath / fileName;
            const FilePath newTgtFilePath = tgtFilePath / fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath, error, copyHelper))
                return false;
        }
    } else {
        if (copyHelper) {
            if (!copyHelper(srcFileInfo, tgtFilePath.toFileInfo(), error))
                return false;
        } else {
            if (!QFile::copy(srcFilePath.toString(), tgtFilePath.toString())) {
                if (error) {
                    *error = QCoreApplication::translate("Utils::FileUtils", "Could not copy file \"%1\" to \"%2\".")
                            .arg(srcFilePath.toUserOutput(), tgtFilePath.toUserOutput());
                }
                return false;
            }
        }
    }
    return true;
}

/*!
  If this is a directory, the function will recursively check all files and return
  true if one of them is newer than \a timeStamp. If this is a single file, true will
  be returned if the file is newer than \a timeStamp.

  Returns whether at least one file in \a filePath has a newer date than
  \a timeStamp.
*/
bool FilePath::isNewerThan(const QDateTime &timeStamp) const
{
    const QFileInfo fileInfo = toFileInfo();
    if (!fileInfo.exists() || fileInfo.lastModified() >= timeStamp)
        return true;
    if (fileInfo.isDir()) {
        const QStringList dirContents = QDir(toString())
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &curFileName : dirContents) {
            if (pathAppended(curFileName).isNewerThan(timeStamp))
                return true;
        }
    }
    return false;
}

/*!
  Recursively resolves symlinks if \a filePath is a symlink.
  To resolve symlinks anywhere in the path, see canonicalPath.
  Unlike QFileInfo::canonicalFilePath(), this function will still return the expected deepest
  target file even if the symlink is dangling.

  \note Maximum recursion depth == 16.

  Returns the symlink target file path.
*/
FilePath FileUtils::resolveSymlinks(const FilePath &path)
{
    QFileInfo f = path.toFileInfo();
    int links = 16;
    while (links-- && f.isSymLink())
        f.setFile(f.dir(), f.symLinkTarget());
    if (links <= 0)
        return FilePath();
    return FilePath::fromString(f.filePath());
}

/*!
  Recursively resolves possibly present symlinks in this file name.
  Unlike QFileInfo::canonicalFilePath(), this function will not return an empty
  string if path doesn't exist.

  Returns the canonical path.
*/
FilePath FilePath::canonicalPath() const
{
    const QString result = toFileInfo().canonicalFilePath();
    if (result.isEmpty())
        return *this;
    return FilePath::fromString(result);
}

FilePath FilePath::operator/(const QString &str) const
{
    return pathAppended(str);
}

/*!
  Like QDir::toNativeSeparators(), but use prefix '~' instead of $HOME on unix systems when an
  absolute path is given.

  Returns the possibly shortened path with native separators.
*/
QString FilePath::shortNativePath() const
{
    if (HostOsInfo::isAnyUnixHost()) {
        const FilePath home = FilePath::fromString(QDir::cleanPath(QDir::homePath()));
        if (isChildOf(home)) {
            return QLatin1Char('~') + QDir::separator()
                + QDir::toNativeSeparators(relativeChildPath(home).toString());
        }
    }
    return toUserOutput();
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
    const QString filePath = path.toString();
    return QFile::setPermissions(filePath, QFile::permissions(filePath) | QFile::WriteUser);
}

// makes sure that capitalization of directories is canonical on Windows and OS X.
// This mimics the logic in QDeclarative_isFileCaseCorrect
QString FileUtils::normalizePathName(const QString &name)
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
#elif defined(Q_OS_OSX)
    return Internal::normalizePathName(name);
#else // do not try to handle case-insensitive file systems on Linux
    return name;
#endif
}

bool FileUtils::isRelativePath(const QString &path)
{
    if (path.startsWith(QLatin1Char('/')))
        return false;
    if (HostOsInfo::isWindowsHost()) {
        if (path.startsWith(QLatin1Char('\\')))
            return false;
        // Unlike QFileInfo, this won't accept a relative path with a drive letter.
        // Such paths result in a royal mess anyway ...
        if (path.length() >= 3 && path.at(1) == QLatin1Char(':') && path.at(0).isLetter()
                && (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\')))
            return false;
    }
    return true;
}

FilePath FilePath::resolvePath(const QString &fileName) const
{
    if (fileName.isEmpty())
        return {}; // FIXME: Isn't this odd?
    if (FileUtils::isAbsolutePath(fileName))
        return FilePath::fromString(QDir::cleanPath(fileName));
    return FilePath::fromString(QDir::cleanPath(toString() + QLatin1Char('/') + fileName));
}

FilePath FileUtils::commonPath(const FilePath &oldCommonPath, const FilePath &filePath)
{
    FilePath newCommonPath = oldCommonPath;
    while (!newCommonPath.isEmpty() && !filePath.isChildOf(newCommonPath))
        newCommonPath = newCommonPath.parentDir();
    return newCommonPath.canonicalPath();
}

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

QByteArray FileReader::fetchQrc(const QString &fileName)
{
    QTC_ASSERT(fileName.startsWith(QLatin1Char(':')), return QByteArray());
    QFile file(fileName);
    bool ok = file.open(QIODevice::ReadOnly);
    QTC_ASSERT(ok, qWarning() << fileName << "not there!"; return QByteArray());
    return file.readAll();
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode)
{
    QTC_ASSERT(!(mode & ~(QIODevice::ReadOnly | QIODevice::Text)), return false);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | mode)) {
        m_errorString = tr("Cannot open %1 for reading: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        return false;
    }
    m_data = file.readAll();
    if (file.error() != QFile::NoError) {
        m_errorString = tr("Cannot read %1: %2").arg(
                QDir::toNativeSeparators(fileName), file.errorString());
        return false;
    }
    return true;
}

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode, QString *errorString)
{
    if (fetch(fileName, mode))
        return true;
    if (errorString)
        *errorString = m_errorString;
    return false;
}

#ifdef QT_GUI_LIB
bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode, QWidget *parent)
{
    if (fetch(fileName, mode))
        return true;
    if (parent)
        QMessageBox::critical(parent, tr("File Error"), m_errorString);
    return false;
}
#endif // QT_GUI_LIB

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
            m_errorString = tr("Cannot write file %1: %2").arg(
                        QDir::toNativeSeparators(m_fileName), m_file->errorString());
        } else {
            m_errorString = tr("Cannot write file %1. Disk full?").arg(
                        QDir::toNativeSeparators(m_fileName));
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


FileSaver::FileSaver(const QString &filename, QIODevice::OpenMode mode)
{
    m_fileName = filename;
    // Workaround an assert in Qt -- and provide a useful error message, too:
    if (HostOsInfo::isWindowsHost()) {
        // Taken from: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
        static const QStringList reservedNames
                = {"CON", "PRN", "AUX", "NUL",
                   "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                   "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
        const QString fn = QFileInfo(filename).baseName().toUpper();
        for (const QString &rn : reservedNames) {
            if (fn == rn) {
                m_errorString = tr("%1: Is a reserved filename on Windows. Cannot save.").arg(filename);
                m_hasError = true;
                return;
            }
        }
    }
    if (mode & (QIODevice::ReadOnly | QIODevice::Append)) {
        m_file.reset(new QFile{filename});
        m_isSafe = false;
    } else {
        m_file.reset(new SaveFile{filename});
        m_isSafe = true;
    }
    if (!m_file->open(QIODevice::WriteOnly | mode)) {
        QString err = QFile::exists(filename) ?
                tr("Cannot overwrite file %1: %2") : tr("Cannot create file %1: %2");
        m_errorString = err.arg(QDir::toNativeSeparators(filename), m_file->errorString());
        m_hasError = true;
    }
}

bool FileSaver::finalize()
{
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
    m_fileName = tempFile->fileName();
}

TempFileSaver::~TempFileSaver()
{
    m_file.reset();
    if (m_autoRemove)
        QFile::remove(m_fileName);
}

/*! \class Utils::FilePath

    \brief The FilePath class is a light-weight convenience class for filenames.

    On windows filenames are compared case insensitively.
*/

FilePath::FilePath()
{
}

/// Constructs a FilePath from \a info
FilePath FilePath::fromFileInfo(const QFileInfo &info)
{
    return FilePath::fromString(info.absoluteFilePath());
}

/// \returns a QFileInfo
QFileInfo FilePath::toFileInfo() const
{
    return QFileInfo(m_data);
}

FilePath FilePath::fromUrl(const QUrl &url)
{
    FilePath fn;
    fn.m_url = url;
    fn.m_data = url.path();
    return fn;
}

/// \returns a QString for passing on to QString based APIs
const QString &FilePath::toString() const
{
    return m_data;
}

QUrl FilePath::toUrl() const
{
    return m_url;
}

/// \returns a QString to display to the user
/// Converts the separators to the native format
QString FilePath::toUserOutput() const
{
    if (m_url.isEmpty())
        return QDir::toNativeSeparators(toString());
    return m_url.toString();
}

QString FilePath::fileName() const
{
    const QChar slash = QLatin1Char('/');
    return m_data.mid(m_data.lastIndexOf(slash) + 1);
}

QString FilePath::fileNameWithPathComponents(int pathComponents) const
{
    if (pathComponents < 0)
        return m_data;
    const QChar slash = QLatin1Char('/');
    int i = m_data.lastIndexOf(slash);
    if (pathComponents == 0 || i == -1)
        return m_data.mid(i + 1);
    int component = i + 1;
    // skip adjacent slashes
    while (i > 0 && m_data.at(--i) == slash)
        ;
    while (i >= 0 && --pathComponents >= 0) {
        i = m_data.lastIndexOf(slash, i);
        component = i + 1;
        while (i > 0 && m_data.at(--i) == slash)
            ;
    }

    // If there are no more slashes before the found one, return the entire string
    if (i > 0 && m_data.lastIndexOf(slash, i) != -1)
        return m_data.mid(component);
    return m_data;
}

/// \returns a bool indicating whether a file with this
/// FilePath exists.
bool FilePath::exists() const
{
    return !isEmpty() && QFileInfo::exists(m_data);
}

/// \returns a bool indicating whether a path is writable.
bool FilePath::isWritablePath() const
{
    const QFileInfo fi{m_data};
    return exists() && fi.isDir() && fi.isWritable();
}

/// Find the parent directory of a given directory.

/// Returns an empty FilePath if the current directory is already
/// a root level directory.

/// \returns \a FilePath with the last segment removed.
FilePath FilePath::parentDir() const
{
    const QString basePath = toString();
    if (basePath.isEmpty())
        return FilePath();

    const QDir base(basePath);
    if (base.isRoot())
        return FilePath();

    const QString path = basePath + QLatin1String("/..");
    const QString parent = QDir::cleanPath(path);
    QTC_ASSERT(parent != path, return FilePath());

    return FilePath::fromString(parent);
}

FilePath FilePath::absolutePath() const
{
    FilePath result = *this;
    result.m_data = QFileInfo(m_data).absolutePath();
    return result;
}

/// Constructs a FilePath from \a filename
/// \a filename is not checked for validity.
FilePath FilePath::fromString(const QString &filename)
{
    FilePath fn;
    fn.m_data = filename;
    return fn;
}

/// Constructs a FilePath from \a filePath. The \a defaultExtension is appended
/// to \a filename if that does not have an extension already.
/// \a filePath is not checked for validity.
FilePath FilePath::fromStringWithExtension(const QString &filepath, const QString &defaultExtension)
{
    if (filepath.isEmpty() || defaultExtension.isEmpty())
        return FilePath::fromString(filepath);

    QString rc = filepath;
    QFileInfo fi(filepath);
    // Add extension unless user specified something else
    const QChar dot = QLatin1Char('.');
    if (!fi.fileName().contains(dot)) {
        if (!defaultExtension.startsWith(dot))
            rc += dot;
        rc += defaultExtension;
    }
    return FilePath::fromString(rc);
}

/// Constructs a FilePath from \a filePath
/// \a filePath is only passed through QDir::fromNativeSeparators
FilePath FilePath::fromUserInput(const QString &filePath)
{
    QString clean = QDir::fromNativeSeparators(filePath);
    if (clean.startsWith(QLatin1String("~/")))
        clean = QDir::homePath() + clean.mid(1);
    return FilePath::fromString(clean);
}

/// Constructs a FilePath from \a filePath, which is encoded as UTF-8.
/// \a filePath is not checked for validity.
FilePath FilePath::fromUtf8(const char *filename, int filenameSize)
{
    return FilePath::fromString(QString::fromUtf8(filename, filenameSize));
}

FilePath FilePath::fromVariant(const QVariant &variant)
{
    if (variant.type() == QVariant::Url)
        return FilePath::fromUrl(variant.toUrl());
    return FilePath::fromString(variant.toString());
}

QVariant FilePath::toVariant() const
{
    if (!m_url.isEmpty())
        return m_url;
    return m_data;
}

bool FilePath::operator==(const FilePath &other) const
{
    if (!m_url.isEmpty())
        return m_url == other.m_url;
    return QString::compare(m_data, other.m_data, HostOsInfo::fileNameCaseSensitivity()) == 0;
}

bool FilePath::operator!=(const FilePath &other) const
{
    return !(*this == other);
}

bool FilePath::operator<(const FilePath &other) const
{
    if (!m_url.isEmpty())
        return m_url < other.m_url;
    return QString::compare(m_data, other.m_data, HostOsInfo::fileNameCaseSensitivity()) < 0;
}

bool FilePath::operator<=(const FilePath &other) const
{
    return !(other < *this);
}

bool FilePath::operator>(const FilePath &other) const
{
    return other < *this;
}

bool FilePath::operator>=(const FilePath &other) const
{
    return !(*this < other);
}

FilePath FilePath::operator+(const QString &s) const
{
    return FilePath::fromString(m_data + s);
}

/// \returns whether FilePath is a child of \a s
bool FilePath::isChildOf(const FilePath &s) const
{
    if (s.isEmpty())
        return false;
    if (!m_data.startsWith(s.m_data, HostOsInfo::fileNameCaseSensitivity()))
        return false;
    if (m_data.size() <= s.m_data.size())
        return false;
    // s is root, '/' was already tested in startsWith
    if (s.m_data.endsWith(QLatin1Char('/')))
        return true;
    // s is a directory, next character should be '/' (/tmpdir is NOT a child of /tmp)
    return m_data.at(s.m_data.size()) == QLatin1Char('/');
}

/// \overload
bool FilePath::isChildOf(const QDir &dir) const
{
    return isChildOf(FilePath::fromString(dir.absolutePath()));
}

/// \returns whether FilePath startsWith \a s
bool FilePath::startsWith(const QString &s) const
{
    return m_data.startsWith(s, HostOsInfo::fileNameCaseSensitivity());
}

/// \returns whether FilePath endsWith \a s
bool FilePath::endsWith(const QString &s) const
{
    return m_data.endsWith(s, HostOsInfo::fileNameCaseSensitivity());
}

bool FilePath::isDir() const
{
    QTC_CHECK(m_url.isEmpty()); // FIXME: Not implemented yet.
    return QFileInfo(m_data).isDir();
}

/// \returns the relativeChildPath of FilePath to parent if FilePath is a child of parent
/// \note returns a empty FilePath if FilePath is not a child of parent
/// That is, this never returns a path starting with "../"
FilePath FilePath::relativeChildPath(const FilePath &parent) const
{
    if (!isChildOf(parent))
        return FilePath();
    return FilePath::fromString(m_data.mid(parent.m_data.size() + 1, -1));
}

FilePath FilePath::pathAppended(const QString &str) const
{
    FilePath fn = *this;
    if (str.isEmpty())
        return fn;
    if (!isEmpty() && !m_data.endsWith(QLatin1Char('/')))
        fn.m_data.append('/');
    fn.m_data.append(str);
    return fn;
}

FilePath FilePath::stringAppended(const QString &str) const
{
    FilePath fn = *this;
    fn.m_data.append(str);
    return fn;
}

uint FilePath::hash(uint seed) const
{
    if (Utils::HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
        return qHash(m_data.toUpper(), seed);
    return qHash(m_data, seed);
}

QTextStream &operator<<(QTextStream &s, const FilePath &fn)
{
    return s << fn.toString();
}

#ifdef QT_GUI_LIB
FileUtils::CopyAskingForOverwrite::CopyAskingForOverwrite(QWidget *dialogParent)
    : m_parent(dialogParent)
{}

bool FileUtils::CopyAskingForOverwrite::operator()(const QFileInfo &src,
                                                   const QFileInfo &dest,
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
                    .arg(FilePath::fromFileInfo(dest).toUserOutput()),
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
                QFile::remove(dest.filePath());
        }
    }
    if (copyFile) {
        if (!dest.absoluteDir().exists())
            dest.absoluteDir().mkpath(dest.absolutePath());
        if (!QFile::copy(src.filePath(), dest.filePath())) {
            if (error) {
                *error = QCoreApplication::translate("Utils::FileUtils",
                                                     "Could not copy file \"%1\" to \"%2\".")
                             .arg(FilePath::fromFileInfo(src).toUserOutput(),
                                  FilePath::fromFileInfo(dest).toUserOutput());
            }
            return false;
        }
    }
    m_files.append(dest.absoluteFilePath());
    return true;
}

QStringList FileUtils::CopyAskingForOverwrite::files() const
{
    return m_files;
}
#endif // QT_GUI_LIB

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

std::hash<Utils::FilePath>::result_type
    std::hash<Utils::FilePath>::operator()(const std::hash<Utils::FilePath>::argument_type &fn) const
{
    if (Utils::HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
        return hash<string>()(fn.toString().toUpper().toStdString());
    return hash<string>()(fn.toString().toStdString());
}
