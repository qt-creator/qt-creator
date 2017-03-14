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
#include "hostosinfo.h"
#include "qtcassert.h"

#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QRegExp>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <shlobj.h>
#endif

#ifdef Q_OS_OSX
#include "fileutils_mac.h"
#endif

QT_BEGIN_NAMESPACE
QDebug operator<<(QDebug dbg, const Utils::FileName &c)
{
    return dbg << c.toString();
}

QT_END_NAMESPACE

namespace Utils {

/*! \class Utils::FileUtils

  \brief The FileUtils class contains file and directory related convenience
  functions.

*/

/*!
  Removes the directory \a filePath and its subdirectories recursively.

  \note The \a error parameter is optional.

  Returns whether the operation succeeded.
*/
bool FileUtils::removeRecursively(const FileName &filePath, QString *error)
{
    QFileInfo fileInfo = filePath.toFileInfo();
    if (!fileInfo.exists() && !fileInfo.isSymLink())
        return true;
    QFile::setPermissions(filePath.toString(), fileInfo.permissions() | QFile::WriteUser);
    if (fileInfo.isDir()) {
        QDir dir(filePath.toString());
        dir = dir.canonicalPath();
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

        QStringList fileNames = dir.entryList(QDir::Files | QDir::Hidden
                                              | QDir::System | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &fileName, fileNames) {
            if (!removeRecursively(FileName(filePath).appendPath(fileName), error))
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
bool FileUtils::copyRecursively(const FileName &srcFilePath, const FileName &tgtFilePath,
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
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot
                                                    | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames) {
            FileName newSrcFilePath = srcFilePath;
            newSrcFilePath.appendPath(fileName);
            FileName newTgtFilePath = tgtFilePath;
            newTgtFilePath.appendPath(fileName);
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
  If \a filePath is a directory, the function will recursively check all files and return
  true if one of them is newer than \a timeStamp. If \a filePath is a single file, true will
  be returned if the file is newer than \a timeStamp.

  Returns whether at least one file in \a filePath has a newer date than
  \a timeStamp.
*/
bool FileUtils::isFileNewerThan(const FileName &filePath, const QDateTime &timeStamp)
{
    QFileInfo fileInfo = filePath.toFileInfo();
    if (!fileInfo.exists() || fileInfo.lastModified() >= timeStamp)
        return true;
    if (fileInfo.isDir()) {
        const QStringList dirContents = QDir(filePath.toString())
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QString &curFileName, dirContents) {
            if (isFileNewerThan(FileName(filePath).appendPath(curFileName), timeStamp))
                return true;
        }
    }
    return false;
}

/*!
  Recursively resolves symlinks if \a filePath is a symlink.
  To resolve symlinks anywhere in the path, see canonicalPath

  \note Maximum recursion depth == 16.

  Returns the symlink target file path.
*/
FileName FileUtils::resolveSymlinks(const FileName &path)
{
    QFileInfo f = path.toFileInfo();
    int links = 16;
    while (links-- && f.isSymLink())
        f.setFile(f.symLinkTarget());
    if (links <= 0)
        return FileName();
    return FileName::fromString(f.filePath());
}

/*!
  Recursively resolves possibly present symlinks in \a filePath.
  Unlike QFileInfo::canonicalFilePath(), this function will not return an empty
  string if path doesn't exist.

  Returns the canonical path.
*/
FileName FileUtils::canonicalPath(const FileName &path)
{
    const QString result = QFileInfo(path.toString()).canonicalFilePath();
    if (result.isEmpty())
        return path;
    return FileName::fromString(result);
}

/*!
  Like QDir::toNativeSeparators(), but use prefix '~' instead of $HOME on unix systems when an
  absolute path is given.

  Returns the possibly shortened path with native separators.
*/
QString FileUtils::shortNativePath(const FileName &path)
{
    if (HostOsInfo::isAnyUnixHost()) {
        const FileName home = FileName::fromString(QDir::cleanPath(QDir::homePath()));
        if (path.isChildOf(home)) {
            return QLatin1Char('~') + QDir::separator()
                + QDir::toNativeSeparators(path.relativeChildPath(home).toString());
        }
    }
    return path.toUserOutput();
}

QString FileUtils::fileSystemFriendlyName(const QString &name)
{
    QString result = name;
    result.replace(QRegExp(QLatin1String("\\W")), QLatin1String("_"));
    result.replace(QRegExp(QLatin1String("_+")), QLatin1String("_")); // compact _
    result.remove(QRegExp(QLatin1String("^_*"))); // remove leading _
    result.remove(QRegExp(QLatin1String("_+$"))); // remove trailing _
    if (result.isEmpty())
        result = QLatin1String("unknown");
    return result;
}

int FileUtils::indexOfQmakeUnfriendly(const QString &name, int startpos)
{
    static QRegExp checkRegExp(QLatin1String("[^a-zA-Z0-9_.-]"));
    return checkRegExp.indexIn(name, startpos);
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

bool FileUtils::makeWritable(const FileName &path)
{
    const QString fileName = path.toString();
    return QFile::setPermissions(fileName, QFile::permissions(fileName) | QFile::WriteUser);
}

// makes sure that capitalization of directories is canonical on Windows and OS X.
// This mimics the logic in QDeclarative_isFileCaseCorrect
QString FileUtils::normalizePathName(const QString &name)
{
#ifdef Q_OS_WIN
    const QString nativeSeparatorName(QDir::toNativeSeparators(name));
    const LPCTSTR nameC = reinterpret_cast<LPCTSTR>(nativeSeparatorName.utf16()); // MinGW
    PIDLIST_ABSOLUTE file;
    HRESULT hr = SHParseDisplayName(nameC, NULL, &file, 0, NULL);
    if (FAILED(hr))
        return name;
    TCHAR buffer[MAX_PATH];
    if (!SHGetPathFromIDList(file, buffer))
        return name;
    return QDir::fromNativeSeparators(QString::fromUtf16(reinterpret_cast<const ushort *>(buffer)));
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

QString FileUtils::resolvePath(const QString &baseDir, const QString &fileName)
{
    if (fileName.isEmpty())
        return QString();
    if (isAbsolutePath(fileName))
        return QDir::cleanPath(fileName);
    return QDir::cleanPath(baseDir + QLatin1Char('/') + fileName);
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

bool FileReader::fetch(const QString &fileName, QIODevice::OpenMode mode, QWidget *parent)
{
    if (fetch(fileName, mode))
        return true;
    if (parent)
        QMessageBox::critical(parent, tr("File Error"), m_errorString);
    return false;
}


FileSaverBase::FileSaverBase()
    : m_hasError(false)
{
}

FileSaverBase::~FileSaverBase()
{
    delete m_file;
}

bool FileSaverBase::finalize()
{
    m_file->close();
    setResult(m_file->error() == QFile::NoError);
    // We delete the object, so it is really closed even if it is a QTemporaryFile.
    delete m_file;
    m_file = 0;
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

bool FileSaverBase::finalize(QWidget *parent)
{
    if (finalize())
        return true;
    QMessageBox::critical(parent, tr("File Error"), errorString());
    return false;
}

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
        m_errorString = tr("Cannot write file %1. Disk full?").arg(
                QDir::toNativeSeparators(m_fileName));
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
        m_file = new QFile(filename);
        m_isSafe = false;
    } else {
        m_file = new SaveFile(filename);
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

    SaveFile *sf = static_cast<SaveFile *>(m_file);
    if (m_hasError) {
        if (sf->isOpen())
            sf->rollback();
    } else {
        setResult(sf->commit());
    }
    delete sf;
    m_file = 0;
    return !m_hasError;
}

TempFileSaver::TempFileSaver(const QString &templ)
    : m_autoRemove(true)
{
    QTemporaryFile *tempFile = new QTemporaryFile();
    if (!templ.isEmpty())
        tempFile->setFileTemplate(templ);
    tempFile->setAutoRemove(false);
    if (!tempFile->open()) {
        m_errorString = tr("Cannot create temporary file in %1: %2").arg(
                QDir::toNativeSeparators(QFileInfo(tempFile->fileTemplate()).absolutePath()),
                tempFile->errorString());
        m_hasError = true;
    }
    m_file = tempFile;
    m_fileName = tempFile->fileName();
}

TempFileSaver::~TempFileSaver()
{
    delete m_file;
    m_file = 0;
    if (m_autoRemove)
        QFile::remove(m_fileName);
}

/*! \class Utils::FileName

    \brief The FileName class is a light-weight convenience class for filenames.

    On windows filenames are compared case insensitively.
*/

FileName::FileName()
    : QString()
{

}

/// Constructs a FileName from \a info
FileName::FileName(const QFileInfo &info)
    : QString(info.absoluteFilePath())
{
}

/// \returns a QFileInfo
QFileInfo FileName::toFileInfo() const
{
    return QFileInfo(*this);
}

/// \returns a QString for passing on to QString based APIs
const QString &FileName::toString() const
{
    return *this;
}

/// \returns a QString to display to the user
/// Converts the separators to the native format
QString FileName::toUserOutput() const
{
    return QDir::toNativeSeparators(toString());
}

QString FileName::fileName(int pathComponents) const
{
    if (pathComponents < 0)
        return *this;
    const QChar slash = QLatin1Char('/');
    QTC_CHECK(!endsWith(slash));
    int i = lastIndexOf(slash);
    if (pathComponents == 0 || i == -1)
        return mid(i + 1);
    int component = i + 1;
    // skip adjacent slashes
    while (i > 0 && at(--i) == slash);
    while (i >= 0 && --pathComponents >= 0) {
        i = lastIndexOf(slash, i);
        component = i + 1;
        while (i > 0 && at(--i) == slash);
    }

    // If there are no more slashes before the found one, return the entire string
    if (i > 0 && lastIndexOf(slash, i) != -1)
        return mid(component);
    return *this;
}

/// \returns a bool indicating whether a file with this
/// FileName exists.
bool FileName::exists() const
{
    return QFileInfo::exists(*this);
}

/// Find the parent directory of a given directory.

/// Returns an empty FileName if the current directory is already
/// a root level directory.

/// \returns \a FileName with the last segment removed.
FileName FileName::parentDir() const
{
    const QString basePath = toString();
    if (basePath.isEmpty())
        return FileName();

    const QDir base(basePath);
    if (base.isRoot())
        return FileName();

    const QString path = basePath + QLatin1String("/..");
    const QString parent = QDir::cleanPath(path);

    return FileName::fromString(parent);
}

/// Constructs a FileName from \a filename
/// \a filename is not checked for validity.
FileName FileName::fromString(const QString &filename)
{
    return FileName(filename);
}

/// Constructs a FileName from \a fileName. The \a defaultExtension is appended
/// to \a filename if that does not have an extension already.
/// \a fileName is not checked for validity.
FileName FileName::fromString(const QString &filename, const QString &defaultExtension)
{
    if (filename.isEmpty() || defaultExtension.isEmpty())
        return filename;

    QString rc = filename;
    QFileInfo fi(filename);
    // Add extension unless user specified something else
    const QChar dot = QLatin1Char('.');
    if (!fi.fileName().contains(dot)) {
        if (!defaultExtension.startsWith(dot))
            rc += dot;
        rc += defaultExtension;
    }
    return rc;
}

/// Constructs a FileName from \a fileName
/// \a fileName is not checked for validity.
FileName FileName::fromLatin1(const QByteArray &filename)
{
    return FileName(QString::fromLatin1(filename));
}

/// Constructs a FileName from \a fileName
/// \a fileName is only passed through QDir::cleanPath
FileName FileName::fromUserInput(const QString &filename)
{
    QString clean = QDir::cleanPath(filename);
    if (clean.startsWith(QLatin1String("~/")))
        clean = QDir::homePath() + clean.mid(1);
    return FileName(clean);
}

/// Constructs a FileName from \a fileName, which is encoded as UTF-8.
/// \a fileName is not checked for validity.
FileName FileName::fromUtf8(const char *filename, int filenameSize)
{
    return FileName(QString::fromUtf8(filename, filenameSize));
}

FileName::FileName(const QString &string)
    : QString(string)
{

}

bool FileName::operator==(const FileName &other) const
{
    return QString::compare(*this, other, HostOsInfo::fileNameCaseSensitivity()) == 0;
}

bool FileName::operator!=(const FileName &other) const
{
    return !(*this == other);
}

bool FileName::operator<(const FileName &other) const
{
    return QString::compare(*this, other, HostOsInfo::fileNameCaseSensitivity()) < 0;
}

bool FileName::operator<=(const FileName &other) const
{
    return QString::compare(*this, other, HostOsInfo::fileNameCaseSensitivity()) <= 0;
}

bool FileName::operator>(const FileName &other) const
{
    return other < *this;
}

bool FileName::operator>=(const FileName &other) const
{
    return other <= *this;
}

/// \returns whether FileName is a child of \a s
bool FileName::isChildOf(const FileName &s) const
{
    if (s.isEmpty())
        return false;
    if (!QString::startsWith(s, HostOsInfo::fileNameCaseSensitivity()))
        return false;
    if (size() <= s.size())
        return false;
    // s is root, '/' was already tested in startsWith
    if (s.QString::endsWith(QLatin1Char('/')))
        return true;
    // s is a directory, next character should be '/' (/tmpdir is NOT a child of /tmp)
    return at(s.size()) == QLatin1Char('/');
}

/// \overload
bool FileName::isChildOf(const QDir &dir) const
{
    return isChildOf(FileName::fromString(dir.absolutePath()));
}

/// \returns whether FileName endsWith \a s
bool FileName::endsWith(const QString &s) const
{
    return QString::endsWith(s, HostOsInfo::fileNameCaseSensitivity());
}

/// \returns the relativeChildPath of FileName to parent if FileName is a child of parent
/// \note returns a empty FileName if FileName is not a child of parent
/// That is, this never returns a path starting with "../"
FileName FileName::relativeChildPath(const FileName &parent) const
{
    if (!isChildOf(parent))
        return FileName();
    return FileName(QString::mid(parent.size() + 1, -1));
}

/// Appends \a s, ensuring a / between the parts
FileName &FileName::appendPath(const QString &s)
{
    if (s.isEmpty())
        return *this;
    if (!isEmpty() && !QString::endsWith(QLatin1Char('/')))
        appendString(QLatin1Char('/'));
    appendString(s);
    return *this;
}

FileName &FileName::appendString(const QString &str)
{
    QString::append(str);
    return *this;
}

FileName &FileName::appendString(QChar str)
{
    QString::append(str);
    return *this;
}

QTextStream &operator<<(QTextStream &s, const FileName &fn)
{
    return s << fn.toString();
}

} // namespace Utils

QT_BEGIN_NAMESPACE
uint qHash(const Utils::FileName &a)
{
    if (Utils::HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive)
        return qHash(a.toString().toUpper());
    return qHash(a.toString());
}
QT_END_NAMESPACE
