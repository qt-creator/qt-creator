// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filestreamermanager.h"

#include "filestreamer.h"
#include "threadutils.h"
#include "utilstr.h"

#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>

#include <unordered_map>

namespace Utils {

// TODO: destruct the instance before destructing ProjectExplorer::DeviceManager (?)

static FileStreamHandle generateUniqueHandle()
{
    static std::atomic_int handleCounter = 1;
    return FileStreamHandle(handleCounter.fetch_add(1));
}

static QMutex s_mutex = {};
static QWaitCondition s_waitCondition = {};
static std::unordered_map<FileStreamHandle, FileStreamer *> s_fileStreamers = {};

static void addStreamer(FileStreamHandle handle, FileStreamer *streamer)
{
    QMutexLocker locker(&s_mutex);
    const bool added = s_fileStreamers.try_emplace(handle, streamer).second;
    QTC_CHECK(added);
}

static void removeStreamer(FileStreamHandle handle)
{
    QMutexLocker locker(&s_mutex);
    auto it = s_fileStreamers.find(handle);
    QTC_ASSERT(it != s_fileStreamers.end(), return);
    QTC_ASSERT(QThread::currentThread() == it->second->thread(), return);
    s_fileStreamers.erase(it);
    s_waitCondition.wakeAll();
}

static void deleteStreamer(FileStreamHandle handle)
{
    QMutexLocker locker(&s_mutex);
    auto it = s_fileStreamers.find(handle);
    if (it == s_fileStreamers.end())
        return;
    if (QThread::currentThread() == it->second->thread()) {
        delete it->second;
        s_fileStreamers.erase(it);
        s_waitCondition.wakeAll();
    } else {
        QMetaObject::invokeMethod(it->second, [handle] {
            deleteStreamer(handle);
        });
        s_waitCondition.wait(&s_mutex);
        QTC_CHECK(s_fileStreamers.find(handle) == s_fileStreamers.end());
    }
}

static void deleteAllStreamers()
{
    QMutexLocker locker(&s_mutex);
    QTC_ASSERT(Utils::isMainThread(), return);
    while (s_fileStreamers.size()) {
        auto it = s_fileStreamers.begin();
        if (QThread::currentThread() == it->second->thread()) {
            delete it->second;
            s_fileStreamers.erase(it);
            s_waitCondition.wakeAll();
        } else {
            const FileStreamHandle handle = it->first;
            QMetaObject::invokeMethod(it->second, [handle] {
                deleteStreamer(handle);
            });
            s_waitCondition.wait(&s_mutex);
            QTC_CHECK(s_fileStreamers.find(handle) == s_fileStreamers.end());
        }
    }
}

static FileStreamHandle checkHandle(FileStreamHandle handle)
{
    QMutexLocker locker(&s_mutex);
    return s_fileStreamers.find(handle) != s_fileStreamers.end() ? handle : FileStreamHandle(0);
}

FileStreamHandle execute(const std::function<void(FileStreamer *)> &onSetup,
                         const std::function<void(FileStreamer *)> &onDone,
                         QObject *context)
{
    FileStreamer *streamer = new FileStreamer;
    onSetup(streamer);
    const FileStreamHandle handle = generateUniqueHandle();
    QTC_CHECK(context == nullptr || context->thread() == QThread::currentThread());
    if (onDone) {
        QObject *finalContext = context ? context : streamer;
        QObject::connect(streamer, &FileStreamer::done, finalContext, [=] { onDone(streamer); });
    }
    QObject::connect(streamer, &FileStreamer::done, streamer, [=] {
        removeStreamer(handle);
        streamer->deleteLater();
    });
    addStreamer(handle, streamer);
    streamer->start();
    return checkHandle(handle); // The handle could have been already removed
}

FileStreamHandle FileStreamerManager::copy(const FilePath &source, const FilePath &destination,
                                           const CopyContinuation &cont)
{
    return copy(source, destination, nullptr, cont);
}

FileStreamHandle FileStreamerManager::copy(const FilePath &source, const FilePath &destination,
                                           QObject *context, const CopyContinuation &cont)
{
    const auto onSetup = [=](FileStreamer *streamer) {
        streamer->setSource(source);
        streamer->setDestination(destination);
    };
    if (!cont)
        return execute(onSetup, {}, context);

    const auto onDone = [=](FileStreamer *streamer) {
        if (streamer->result() == Tasking::DoneResult::Success)
            cont({});
        else
            cont(ResultError(Tr::tr("Failed copying file.")));
    };
    return execute(onSetup, onDone, context);
}

FileStreamHandle FileStreamerManager::read(const FilePath &source, const ReadContinuation &cont)
{
    return read(source, nullptr, cont);
}

FileStreamHandle FileStreamerManager::read(const FilePath &source, QObject *context,
                                           const ReadContinuation &cont)
{
    const auto onSetup = [=](FileStreamer *streamer) {
        streamer->setStreamMode(StreamMode::Reader);
        streamer->setSource(source);
    };
    if (!cont)
        return execute(onSetup, {}, context);

    const auto onDone = [=](FileStreamer *streamer) {
        if (streamer->result() == Tasking::DoneResult::Success)
            cont(streamer->readData());
        else
            cont(ResultError(Tr::tr("Failed reading file.")));
    };
    return execute(onSetup, onDone, context);
}

FileStreamHandle FileStreamerManager::write(const FilePath &destination, const QByteArray &data,
                                            const WriteContinuation &cont)
{
    return write(destination, data, nullptr, cont);
}

FileStreamHandle FileStreamerManager::write(const FilePath &destination, const QByteArray &data,
                                            QObject *context, const WriteContinuation &cont)
{
    const auto onSetup = [=](FileStreamer *streamer) {
        streamer->setStreamMode(StreamMode::Writer);
        streamer->setDestination(destination);
        streamer->setWriteData(data);
    };
    if (!cont)
        return execute(onSetup, {}, context);

    const auto onDone = [=](FileStreamer *streamer) {
        if (streamer->result() == Tasking::DoneResult::Success)
            cont(0); // TODO: return write count?
        else
            cont(ResultError(Tr::tr("Failed writing file.")));
    };
    return execute(onSetup, onDone, context);
}

void FileStreamerManager::stop(FileStreamHandle handle)
{
    deleteStreamer(handle);
}

void FileStreamerManager::stopAll()
{
    deleteAllStreamers();
}

} // namespace Utils

