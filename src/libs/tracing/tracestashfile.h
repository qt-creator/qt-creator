// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/temporaryfile.h>

#include <QFile>
#include <QDataStream>

#include <memory>

namespace Timeline {

template<typename Event>
class TraceStashFile
{
public:
    TraceStashFile(const QString &pattern) : file(pattern) {}

    bool open()
    {
        if (!file.open())
            return false;

        stream.setDevice(&file);
        return true;
    }

    void append(const Event &event)
    {
        stream << event;
    }

    enum ReplayResult {
        ReplaySuccess,
        ReplayOpenFailed,
        ReplayLoadFailed,
        ReplayReadPastEnd
    };

    class Iterator
    {
    private:
        friend class TraceStashFile<Event>;

        std::unique_ptr<QFile> readFile;
        std::unique_ptr<QDataStream> readStream;
        Event nextEvent;
        bool streamAtEnd;

        bool open()
        {
            if (readFile->open(QIODevice::ReadOnly)) {
                readStream->setDevice(readFile.get());
                if (readStream->atEnd()) {
                    streamAtEnd = true;
                } else {
                    (*readStream) >> nextEvent;
                    if (readPastEnd())
                        streamAtEnd = true;
                }
                return true;
            }
            streamAtEnd = true;
            return false;
        }

        Iterator(const QString &fileName) :
            readFile(std::make_unique<QFile>(fileName)),
            readStream(std::make_unique<QDataStream>()),
            streamAtEnd(false)
        {
        }

        bool readPastEnd() const
        {
            return readStream->status() == QDataStream::ReadPastEnd;
        }

    public:
        Event next()
        {
            if (readStream->atEnd()) {
                streamAtEnd = true;
                return std::move(nextEvent);
            }

            const Event result = std::move(nextEvent);
            (*readStream) >> nextEvent;
            if (readPastEnd())
                streamAtEnd = true;
            return result;
        }

        const Event &peekNext() const
        {
            return nextEvent;
        }

        bool hasNext() const
        {
            return !streamAtEnd;
        }
    };

    Iterator iterator() const
    {
        Iterator i(file.fileName());
        i.open();
        return i;
    }

    template<typename Loader>
    ReplayResult replay(const Loader &loader) const
    {
        Iterator replayIterator(file.fileName());
        if (!replayIterator.open())
            return ReplayOpenFailed;

        while (replayIterator.hasNext()) {
            if (!loader(replayIterator.next()))
                return ReplayLoadFailed;
            if (replayIterator.readPastEnd())
                return ReplayReadPastEnd;
        }

        return ReplaySuccess;
    }

    void clear()
    {
        file.remove();
        stream.setDevice(nullptr);
    }

    bool flush()
    {
        return file.flush();
    }

private:
    Utils::TemporaryFile file;
    QDataStream stream;
};

}
