// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <solutions/tasking/tasktree.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace Utils {

enum class StreamMode { Reader, Writer, Transfer };

enum class StreamResult { FinishedWithSuccess, FinishedWithError };

class QTCREATOR_UTILS_EXPORT FileStreamer final : public QObject
{
    Q_OBJECT

public:
    FileStreamer(QObject *parent = nullptr);
    ~FileStreamer();

    void setSource(const FilePath &source);
    void setDestination(const FilePath &destination);
    void setStreamMode(StreamMode mode); // Transfer by default

    // Only for Reader mode
    QByteArray readData() const;
    // Only for Writer mode
    void setWriteData(const QByteArray &writeData);

    StreamResult result() const;

    void start();
    void stop();

signals:
    void done();

private:
    class FileStreamerPrivate *d = nullptr;
};

class FileStreamerTaskAdapter : public Tasking::TaskAdapter<FileStreamer>
{
public:
    FileStreamerTaskAdapter() { connect(task(), &FileStreamer::done, this,
                [this] { emit done(task()->result() == StreamResult::FinishedWithSuccess); }); }
    void start() override { task()->start(); }
};

using FileStreamerTask = Tasking::CustomTask<FileStreamerTaskAdapter>;

} // namespace Utils
