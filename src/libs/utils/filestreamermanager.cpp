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

using namespace QtTaskTree;

namespace Utils::FileStreamerManager {

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

static FileStreamHandle execute(const std::function<void(FileStreamer *)> &onSetup,
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

FileStreamHandle copy(const Continuation<> &cont,
                      const FilePath &source,
                      const FilePath &destination)
{
    const auto onSetup = [=](FileStreamer *streamer) {
        streamer->setSource(source);
        streamer->setDestination(destination);
    };

    const auto onDone = [=](FileStreamer *streamer) {
        if (streamer->result() == DoneResult::Success)
            cont({});
        else
            cont(ResultError(Tr::tr("Failed copying file.")));
    };
    return execute(onSetup, onDone, cont.guard());
}

FileStreamHandle read(const Continuation<QByteArray> &cont,
                      const FilePath &source)
{
    const auto onSetup = [=](FileStreamer *streamer) {
        streamer->setStreamMode(StreamMode::Reader);
        streamer->setSource(source);
    };

    const auto onDone = [=](FileStreamer *streamer) {
        if (streamer->result() == DoneResult::Success)
            cont(streamer->readData());
        else
            cont(ResultError(Tr::tr("Failed reading file.")));
    };
    return execute(onSetup, onDone, cont.guard());
}


FileStreamHandle write(const Continuation<qint64> &cont,
                       const FilePath &destination,
                       const QByteArray &data)
{
    const auto onSetup = [=](FileStreamer *streamer) {
        streamer->setStreamMode(StreamMode::Writer);
        streamer->setDestination(destination);
        streamer->setWriteData(data);
    };

    const auto onDone = [=](FileStreamer *streamer) {
        if (streamer->result() == DoneResult::Success)
            cont(0); // TODO: return write count?
        else
            cont(ResultError(Tr::tr("Failed writing file.")));
    };
    return execute(onSetup, onDone, cont.guard());
}

void stop(FileStreamHandle handle)
{
    deleteStreamer(handle);
}

void stopAll()
{
    deleteAllStreamers();
}

} // namespace Utils::FileStreamerManager

