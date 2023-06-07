// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "multifiledownloader.h"
#include "filedownloader.h"

namespace QmlDesigner {

MultiFileDownloader::MultiFileDownloader(QObject *parent)
    : QObject(parent)
{}

MultiFileDownloader::~MultiFileDownloader()
{}

void MultiFileDownloader::setDownloader(FileDownloader *downloader)
{
    m_downloader = downloader;

    QObject::connect(this, &MultiFileDownloader::downloadStarting, [this]() {
        m_nextFile = 0;
        if (m_files.length() > 0)
            m_downloader->start();
    });

    QObject::connect(m_downloader, &FileDownloader::progressChanged, this, [this]() {
        double curProgress = m_downloader->progress() / 100.0;
        double totalProgress = (m_nextFile + curProgress) / m_files.size();
        m_progress = totalProgress * 100;
        emit progressChanged();
    });

    QObject::connect(m_downloader, &FileDownloader::downloadFailed, this, [this]() {
        m_failed = true;
        emit downloadFailed();
    });

    QObject::connect(m_downloader, &FileDownloader::downloadCanceled, this, [this]() {
        m_canceled = true;
        emit downloadCanceled();
    });

    QObject::connect(m_downloader, &FileDownloader::finishedChanged, this, [this]() {
        switchToNextFile();
    });
}

FileDownloader *MultiFileDownloader::downloader()
{
    return m_downloader;
}

void MultiFileDownloader::start()
{
    m_canceled = false;
    emit downloadStarting();
}

void MultiFileDownloader::cancel()
{
    m_canceled = true;
    m_downloader->cancel();
}

void MultiFileDownloader::setBaseUrl(const QUrl &baseUrl)
{
    if (m_baseUrl != baseUrl) {
        m_baseUrl = baseUrl;
        emit baseUrlChanged();
    }
}

QUrl MultiFileDownloader::baseUrl() const
{
    return m_baseUrl;
}

bool MultiFileDownloader::finished() const
{
    return m_finished;
}

int MultiFileDownloader::progress() const
{
    return m_progress;
}

void MultiFileDownloader::setTargetDirPath(const QString &path)
{
    m_targetDirPath = path;
}

QString MultiFileDownloader::targetDirPath() const
{
    return m_targetDirPath;
}

QString MultiFileDownloader::nextUrl() const
{
    if (m_nextFile >= m_files.length())
        return {};

    return m_baseUrl.toString() + "/" + m_files[m_nextFile];
}

QString MultiFileDownloader::nextTargetPath() const
{
    if (m_nextFile >= m_files.length())
        return {};

    return m_targetDirPath + "/" + m_files[m_nextFile];
}

void MultiFileDownloader::setFiles(const QStringList &files)
{
    m_files = files;
}

QStringList MultiFileDownloader::files() const
{
    return m_files;
}

void MultiFileDownloader::switchToNextFile()
{
    ++m_nextFile;

    if (m_nextFile < m_files.length()) {
        if (m_canceled) {
            emit downloadCanceled();
        } else {
            emit nextUrlChanged();
            emit nextTargetPathChanged();
            m_downloader->start();
        }
    } else {
        m_finished = true;
        emit finishedChanged();
    }
}

} // namespace QmlDesigner
