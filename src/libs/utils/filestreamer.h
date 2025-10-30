// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QtTaskTree/QTaskTree>

#include <QObject>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace Utils {

enum class StreamMode { Reader, Writer, Transfer };

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

    QtTaskTree::DoneResult result() const;

    void start();
    void stop();

signals:
    void done(QtTaskTree::DoneResult result);

private:
    class FileStreamerPrivate *d = nullptr;
};

using FileStreamerTask = QCustomTask<FileStreamer>;

} // namespace Utils
