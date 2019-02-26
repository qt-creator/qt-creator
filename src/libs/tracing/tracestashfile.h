/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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
