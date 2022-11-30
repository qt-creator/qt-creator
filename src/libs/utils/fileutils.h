// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QCoreApplication>
#include <QDir>

#ifdef QT_WIDGETS_LIB
#include <QFileDialog>
#endif

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class QDataStream;
class QTextStream;
class QWidget;
class QXmlStreamWriter;

// for withNtfsPermissions
#ifdef Q_OS_WIN
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif
QT_END_NAMESPACE

namespace Utils {

class CommandLine;

struct QTCREATOR_UTILS_EXPORT RunResult
{
    int exitCode = -1;
    QByteArray stdOut;
    QByteArray stdErr;
};

class QTCREATOR_UTILS_EXPORT FileUtils
{
public:
#ifdef QT_GUI_LIB
    class QTCREATOR_UTILS_EXPORT CopyAskingForOverwrite
    {
    public:
        CopyAskingForOverwrite(QWidget *dialogParent,
                               const std::function<void(FilePath)> &postOperation = {});
        bool operator()(const FilePath &src, const FilePath &dest, QString *error);
        FilePaths files() const;

    private:
        QWidget *m_parent;
        FilePaths m_files;
        std::function<void(FilePath)> m_postOperation;
        bool m_overwriteAll = false;
        bool m_skipAll = false;
    };
#endif // QT_GUI_LIB

    static bool copyRecursively(const FilePath &srcFilePath,
                                const FilePath &tgtFilePath,
                                QString *error = nullptr);
    template<typename T>
    static bool copyRecursively(const FilePath &srcFilePath,
                                const FilePath &tgtFilePath,
                                QString *error,
                                T &&copyHelper);
    static bool copyIfDifferent(const FilePath &srcFilePath,
                                const FilePath &tgtFilePath);
    static QString fileSystemFriendlyName(const QString &name);
    static int indexOfQmakeUnfriendly(const QString &name, int startpos = 0);
    static QString qmakeFriendlyName(const QString &name);
    static bool makeWritable(const FilePath &path);
    static QString normalizedPathName(const QString &name);

    static FilePath commonPath(const FilePath &oldCommonPath, const FilePath &fileName);
    static FilePath commonPath(const FilePaths &paths);
    static FilePath homePath();

    static FilePaths toFilePathList(const QStringList &paths);

    static qint64 bytesAvailableFromDFOutput(const QByteArray &dfOutput);

    static FilePathInfo filePathInfoFromTriple(const QString &infos);

#ifdef QT_WIDGETS_LIB
    static void setDialogParentGetter(const std::function<QWidget *()> &getter);

    static FilePath getOpenFilePath(QWidget *parent,
                                    const QString &caption,
                                    const FilePath &dir = {},
                                    const QString &filter = {},
                                    QString *selectedFilter = nullptr,
                                    QFileDialog::Options options = {},
                                    bool fromDeviceIfShiftIsPressed = false);

    static FilePath getSaveFilePath(QWidget *parent,
                                    const QString &caption,
                                    const FilePath &dir = {},
                                    const QString &filter = {},
                                    QString *selectedFilter = nullptr,
                                    QFileDialog::Options options = {});

    static FilePath getExistingDirectory(QWidget *parent,
                                         const QString &caption,
                                         const FilePath &dir = {},
                                         QFileDialog::Options options = QFileDialog::ShowDirsOnly,
                                         bool fromDeviceIfShiftIsPressed = false);

    static FilePaths getOpenFilePaths(QWidget *parent,
                                      const QString &caption,
                                      const FilePath &dir = {},
                                      const QString &filter = {},
                                      QString *selectedFilter = nullptr,
                                      QFileDialog::Options options = {});
#endif

};

template<typename T>
bool FileUtils::copyRecursively(const FilePath &srcFilePath,
                                const FilePath &tgtFilePath,
                                QString *error,
                                T &&copyHelper)
{
    if (srcFilePath.isDir()) {
        if (!tgtFilePath.exists()) {
            if (!tgtFilePath.ensureWritableDir()) {
                if (error) {
                    *error = QCoreApplication::translate("Utils::FileUtils",
                                                         "Failed to create directory \"%1\".")
                                 .arg(tgtFilePath.toUserOutput());
                }
                return false;
            }
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

// for actually finding out if e.g. directories are writable on Windows
#ifdef Q_OS_WIN

template <typename T>
T withNtfsPermissions(const std::function<T()> &task)
{
    qt_ntfs_permission_lookup++;
    T result = task();
    qt_ntfs_permission_lookup--;
    return result;
}

template <>
QTCREATOR_UTILS_EXPORT void withNtfsPermissions(const std::function<void()> &task);

#else // Q_OS_WIN

template <typename T>
T withNtfsPermissions(const std::function<T()> &task)
{
    return task();
}

#endif // Q_OS_WIN

class QTCREATOR_UTILS_EXPORT FileReader
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    static QByteArray fetchQrc(const QString &fileName); // Only for internal resources
    bool fetch(const FilePath &filePath, QIODevice::OpenMode mode = QIODevice::NotOpen); // QIODevice::ReadOnly is implicit
    bool fetch(const FilePath &filePath, QIODevice::OpenMode mode, QString *errorString);
    bool fetch(const FilePath &filePath, QString *errorString)
        { return fetch(filePath, QIODevice::NotOpen, errorString); }
#ifdef QT_GUI_LIB
    bool fetch(const FilePath &filePath, QIODevice::OpenMode mode, QWidget *parent);
    bool fetch(const FilePath &filePath, QWidget *parent)
        { return fetch(filePath, QIODevice::NotOpen, parent); }
#endif // QT_GUI_LIB
    const QByteArray &data() const { return m_data; }
    const QString &errorString() const { return m_errorString; }
private:
    QByteArray m_data;
    QString m_errorString;
};

class QTCREATOR_UTILS_EXPORT FileSaverBase
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    FileSaverBase();
    virtual ~FileSaverBase();

    FilePath filePath() const { return m_filePath; }
    bool hasError() const { return m_hasError; }
    QString errorString() const { return m_errorString; }
    virtual bool finalize();
    bool finalize(QString *errStr);
#ifdef QT_GUI_LIB
    bool finalize(QWidget *parent);
#endif

    bool write(const char *data, int len);
    bool write(const QByteArray &bytes);
    bool setResult(QTextStream *stream);
    bool setResult(QDataStream *stream);
    bool setResult(QXmlStreamWriter *stream);
    bool setResult(bool ok);

    QFile *file() { return m_file.get(); }

protected:
    std::unique_ptr<QFile> m_file;
    FilePath m_filePath;
    QString m_errorString;
    bool m_hasError = false;

private:
    Q_DISABLE_COPY(FileSaverBase)
};

class QTCREATOR_UTILS_EXPORT FileSaver : public FileSaverBase
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    // QIODevice::WriteOnly is implicit
    explicit FileSaver(const FilePath &filePath, QIODevice::OpenMode mode = QIODevice::NotOpen);

    bool finalize() override;
    using FileSaverBase::finalize;

private:
    bool m_isSafe = false;
};

class QTCREATOR_UTILS_EXPORT TempFileSaver : public FileSaverBase
{
    Q_DECLARE_TR_FUNCTIONS(Utils::FileUtils) // sic!
public:
    explicit TempFileSaver(const QString &templ = QString());
    ~TempFileSaver() override;

    void setAutoRemove(bool on) { m_autoRemove = on; }

private:
    bool m_autoRemove = true;
};

QTCREATOR_UTILS_EXPORT QTextStream &operator<<(QTextStream &s, const FilePath &fn);

bool isRelativePathHelper(const QString &path, OsType osType);

// For testing
QTCREATOR_UTILS_EXPORT QString doCleanPath(const QString &input);
QTCREATOR_UTILS_EXPORT QString cleanPathHelper(const QString &path);

} // namespace Utils

