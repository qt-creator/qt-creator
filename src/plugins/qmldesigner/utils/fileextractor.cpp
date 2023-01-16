// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "fileextractor.h"

#include <QQmlEngine>
#include <QStorageInfo>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/algorithm.h>
#include <utils/archive.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

namespace QmlDesigner {

FileExtractor::FileExtractor(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(100);
    m_timer.setSingleShot(false);

    QObject::connect(this, &FileExtractor::targetFolderExistsChanged, this, [this]() {
        if (targetFolderExists())
            m_birthTime = QFileInfo(m_targetPath.toString() + "/" + m_archiveName).birthTime();
        else
            m_birthTime = QDateTime();

        emit birthTimeChanged();
    });
}

FileExtractor::~FileExtractor() {}

void FileExtractor::changeTargetPath(const QString &path)
{
    m_targetPath = Utils::FilePath::fromString(path);
    emit targetPathChanged();
    emit targetFolderExistsChanged();
}

QString FileExtractor::targetPath() const
{
    return m_targetPath.toUserOutput();
}

void FileExtractor::setTargetPath(const QString &path)
{
    m_targetPath = Utils::FilePath::fromString(path);
}

void FileExtractor::browse()
{
    const Utils::FilePath path =
        Utils::FileUtils::getExistingDirectory(nullptr, tr("Choose Directory"), m_targetPath);

    if (!path.isEmpty())
        m_targetPath = path;

    emit targetPathChanged();
    emit targetFolderExistsChanged();
}

void FileExtractor::setSourceFile(QString &sourceFilePath)
{
    m_sourceFile = Utils::FilePath::fromString(sourceFilePath);
    emit targetFolderExistsChanged();
}

void FileExtractor::setArchiveName(QString &filePath)
{
    m_archiveName = filePath;
    emit targetFolderExistsChanged();
}

const QString FileExtractor::detailedText()
{
    return m_detailedText;
}

bool FileExtractor::finished() const
{
    return m_finished;
}

QString FileExtractor::currentFile() const
{
    return m_currentFile;
}

QString FileExtractor::size() const
{
    return m_size;
}

QString FileExtractor::count() const
{
    return m_count;
}

bool FileExtractor::targetFolderExists() const
{
    return QFileInfo::exists(m_targetPath.toString() + "/" + m_archiveName);
}

int FileExtractor::progress() const
{
    return m_progress;
}

QDateTime FileExtractor::birthTime() const
{
    return m_birthTime;
}

QString FileExtractor::archiveName() const
{
    return m_archiveName;
}

QString FileExtractor::sourceFile() const
{
    return m_sourceFile.toString();
}

void FileExtractor::extract()
{
    const QString targetFolder = m_targetPath.toString() + "/" + m_archiveName;

    // If the target directory already exists, remove it and its content
    QDir targetDir(targetFolder);
    if (targetDir.exists())
        targetDir.removeRecursively();

    // Create a new directory to generate a proper creation date
    targetDir.mkdir(targetFolder);

    Utils::Archive *archive = new Utils::Archive(m_sourceFile, m_targetPath);
    QTC_ASSERT(archive->isValid(), delete archive; return);

    m_timer.start();
    qint64 bytesBefore = QStorageInfo(m_targetPath.toFileInfo().dir()).bytesAvailable();
    qint64 compressedSize = QFileInfo(m_sourceFile.toString()).size();

    QTimer::connect(
        &m_timer, &QTimer::timeout, this, [this, bytesBefore, targetFolder, compressedSize]() {
            static QHash<QString, int> hash;
            QDirIterator it(targetFolder, {"*.*"}, QDir::Files, QDirIterator::Subdirectories);

            int count = 0;
            while (it.hasNext()) {
                if (!hash.contains(it.fileName())) {
                    m_currentFile = it.fileName();
                    hash.insert(m_currentFile, 0);
                    emit currentFileChanged();
                }
                it.next();
                count++;
            }

            qint64 currentSize = bytesBefore
                                 - QStorageInfo(m_targetPath.toFileInfo().dir()).bytesAvailable();

            // We can not get the uncompressed size of the archive yet, that is why we use an
            // approximation. We assume a 50% compression rate.
            m_progress = std::min(100ll, currentSize * 100 / compressedSize * 2);
            emit progressChanged();

            m_size = QString::number(currentSize);
            m_count = QString::number(count);
            emit sizeChanged();
        });

    QObject::connect(archive, &Utils::Archive::outputReceived, this, [this](const QString &output) {
        m_detailedText += output;
        emit detailedTextChanged();
    });

    QObject::connect(archive, &Utils::Archive::finished, this, [this, archive](bool ret) {
        archive->deleteLater();
        m_finished = ret;
        m_timer.stop();

        m_progress = 100;
        emit progressChanged();

        emit targetFolderExistsChanged();
        emit finishedChanged();
        QTC_CHECK(ret);
    });
    archive->unarchive();
}

} // namespace QmlDesigner
