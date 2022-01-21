/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "commandline.h"
#include "environment.h"
#include "hostosinfo.h"
#include "qtcassert.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QOperatingSystemVersion>
#include <QTimer>
#include <QUrl>
#include <qplatformdefs.h>

#ifdef QT_GUI_LIB
#include <QMessageBox>
#include <QRegularExpression>
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
        // TODO: add error handling to FilePath::fileContents
        m_data = filePath.fileContents();
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
                                    QFileDialog::Options options)
{
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
    return transform(result, &FilePath::fromString);
}

// Used on 'ls' output on unix-like systems.
void FileUtils::iterateLsOutput(const FilePath &base,
                                const QStringList &entries,
                                const FileFilter &filter,
                                const std::function<bool (const FilePath &)> &callBack)
{
    QTC_CHECK(filter.iteratorFlags != QDirIterator::NoIteratorFlags); // FIXME: Not supported yet below.

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

#endif // QT_WIDGETS_LIB

} // namespace Utils
