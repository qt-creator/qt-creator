// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "unarchiver.h"

#include "async.h"
#include "utilstr.h"

#include <QSettings>

#include <archive.h>
#include <archive_entry.h>

using namespace Tasking;

namespace Utils {

static Result<> copy_data(struct archive *ar, struct archive *aw, QPromise<Result<>> &promise)
{
    int r;
    const void *buff;
    size_t size;
    la_int64_t offset;

    while (!promise.isCanceled()) {
        r = archive_read_data_block(ar, &buff, &size, &offset);
        if (r == ARCHIVE_EOF)
            break;
        if (r < ARCHIVE_OK)
            return ResultError(QString::fromUtf8(archive_error_string(ar)));
        r = archive_write_data_block(aw, buff, size, offset);
        if (r < ARCHIVE_OK)
            return ResultError(QString::fromUtf8(archive_error_string(aw)));
    }
    return ResultOk;
}

static void readFree(struct archive *a)
{
    archive_read_close(a);
    archive_read_free(a);
}

static void writeFree(struct archive *a)
{
    archive_write_close(a);
    archive_write_free(a);
}

struct ReadData
{
    QFile file;
    char data[10240];
};

static int _open(struct archive *a, void *client_data)
{
    ReadData *data = static_cast<ReadData *>(client_data);
    if (!data->file.open(QIODevice::ReadOnly)) {
        archive_set_error(a, EIO, "Open error: %s", data->file.errorString().toUtf8().data());
        return ARCHIVE_FATAL;
    }

    return ARCHIVE_OK;
}

static la_ssize_t _read(struct archive *a, void *client_data, const void **buff)
{
    Q_UNUSED(a);
    ReadData *data = static_cast<ReadData *>(client_data);
    *buff = data->data;
    return data->file.read(data->data, 10240);
}

int _close(struct archive *a, void *client_data)
{
    Q_UNUSED(a);
    ReadData *data = static_cast<ReadData *>(client_data);
    data->file.close();
    return ARCHIVE_OK;
}

static int64_t _skip(struct archive *a, void *client_data, int64_t request)
{
    ReadData *data = static_cast<ReadData *>(client_data);
    if (data->file.skip(request))
        return request;
    archive_set_error(a, EIO, "Skip error: %s", data->file.errorString().toUtf8().data());
    return -1;
}

static int64_t _seek(struct archive *a, void *client_data, int64_t request, int whence)
{
    ReadData *data = static_cast<ReadData *>(client_data);
    switch (whence) {
    case SEEK_SET:
        break;
    case SEEK_CUR:
        request += data->file.pos();
        break;
    case SEEK_END:
        request = data->file.size() - request;
        break;
    }
    if (!data->file.seek(request)) {
        archive_set_error(a, EIO, "Seek error: %s", data->file.errorString().toUtf8().data());
        return ARCHIVE_FATAL;
    }
    return data->file.pos();
}

static Result<> unarchive(
    QPromise<Result<>> &promise, const Utils::FilePath &archive, const Utils::FilePath &destination)
{
    struct archive_entry *entry;
    int flags;
    int r;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    ReadData data{QFile(archive.toFSPathString()), {}};
    std::unique_ptr<struct archive, decltype(&readFree)> a(archive_read_new(), readFree);

    if (archive_read_support_format_all(a.get()) != ARCHIVE_OK
        || archive_read_support_format_raw(a.get()) != ARCHIVE_OK
        || archive_read_support_filter_all(a.get()) != ARCHIVE_OK) {
        return ResultError(QString("archive_read_ setup failed: %1")
                                 .arg(QString::fromUtf8(archive_error_string(a.get()))));
    }

    std::unique_ptr<struct archive, decltype(&writeFree)> ext(archive_write_disk_new(), writeFree);

    if (archive_write_disk_set_options(ext.get(), flags) != ARCHIVE_OK
        || archive_write_disk_set_standard_lookup(ext.get()) != ARCHIVE_OK) {
        return ResultError(QString("archive_write_disk_ setup failed: %1")
                                 .arg(QString::fromUtf8(archive_error_string(ext.get()))));
    }

    if (archive_read_append_callback_data(a.get(), &data) != (ARCHIVE_OK))
        return ResultError(
            QString("Could not append callback data to %1").arg(archive.toUserOutput()));

    if (archive_read_set_open_callback(a.get(), _open) != ARCHIVE_OK
        || archive_read_set_read_callback(a.get(), _read) != ARCHIVE_OK
        || archive_read_set_skip_callback(a.get(), _skip) != ARCHIVE_OK
        || archive_read_set_seek_callback(a.get(), _seek) != ARCHIVE_OK
        || archive_read_set_close_callback(a.get(), _close)) {
        return ResultError(QString("Could not set callbacks: %1")
                                 .arg(QString::fromUtf8(archive_error_string(a.get()))));
    }

    if ((r = archive_read_open1(a.get())))
        return ResultError(QString::fromUtf8(archive_error_string(a.get())));

    int fileNumber = 0;
    while (!promise.isCanceled()) {
        r = archive_read_next_header(a.get(), &entry);

        const int format = archive_format(a.get());
        const int filter = archive_filter_code(a.get(), 0);

        if (format == ARCHIVE_FORMAT_RAW && filter == ARCHIVE_FILTER_NONE)
            return ResultError(Tr::tr("Not an archive"));

        if (r == ARCHIVE_EOF)
            break;

        if (r < ARCHIVE_OK) {
            return ResultError(QString::fromUtf8(archive_error_string(a.get())));
        }

        ++fileNumber;
        promise.setProgressRange(0, fileNumber);
        promise.setProgressValueAndText(
            fileNumber, QString::fromUtf8(archive_entry_pathname_utf8(entry)));

        archive_entry_set_pathname_utf8(
            entry,
            (destination / QString::fromUtf8(archive_entry_pathname_utf8(entry))).path().toUtf8());

        r = archive_write_header(ext.get(), entry);
        if (r < ARCHIVE_OK) {
            return ResultError(QString::fromUtf8(archive_error_string(ext.get())));
        } else {
            const struct stat *stat = archive_entry_stat(entry);
            // Is regular file ? (See S_ISREG macro in stat.h)
            if ((((stat->st_mode) & 0170000) == 0100000)) {
                r = copy_data(a.get(), ext.get(), promise).has_value();
                if (r < ARCHIVE_OK) {
                    return ResultError(QString::fromUtf8(archive_error_string(ext.get())));
                }
            }
        }
        r = archive_write_finish_entry(ext.get());
        if (r < ARCHIVE_OK) {
            return ResultError(QString::fromUtf8(archive_error_string(ext.get())));
        }
    }
    return ResultOk;
}

static void unarchivePromised(
    QPromise<Result<>> &promise, const Utils::FilePath &archive, const Utils::FilePath &destination)
{
    promise.addResult(unarchive(promise, archive, destination));
}

Unarchiver::Unarchiver()
{
    m_async.setThreadPool(QThreadPool::globalInstance());

    connect(&m_async, &AsyncBase::started, this, &Unarchiver::started);
    connect(&m_async, &AsyncBase::progressTextChanged, this, [this](const QString &text) {
        emit progress(FilePath::fromString(text));
    });
    connect(&m_async, &AsyncBase::done, this, [this] {
        Result<> result = m_async.isCanceled() ? ResultError(QString()) : m_async.result();
        emit done(result ? DoneResult::Success : DoneResult::Error);
    });
}

void Unarchiver::start()
{
    m_async.setConcurrentCallData(unarchivePromised, m_archive, m_destination);
    m_async.start();
}

Result<> Unarchiver::result() const
{
    if (m_async.isCanceled())
        return ResultError(Tr::tr("Canceled"));
    return m_async.result();
}

void Unarchiver::setArchive(const FilePath &archive)
{
    m_archive = archive;
}

void Unarchiver::setDestination(const FilePath &destination)
{
    m_destination = destination;
}

bool Unarchiver::isDone() const
{
    return m_async.isDone();
}

bool Unarchiver::isCanceled() const
{
    return m_async.isCanceled();
}

} // namespace Utils
