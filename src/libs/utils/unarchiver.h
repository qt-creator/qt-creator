// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "async.h"
#include "filepath.h"

#include <QObject>

namespace Utils {

class QTCREATOR_UTILS_EXPORT Unarchiver : public QObject
{
    Q_OBJECT

public:
    Unarchiver();

    void setArchive(const FilePath &archive);
    void setDestination(const FilePath &destination);

    Result<> result() const;

    void start();

    bool isDone() const;
    bool isCanceled() const;

signals:
    void started();
    void done(Tasking::DoneResult result);
    void progress(const FilePath &path);

private:
    Async<Result<>> m_async;

    FilePath m_archive;
    FilePath m_destination;
};

using UnarchiverTask = Tasking::SimpleCustomTask<Unarchiver>;

} // namespace Utils
