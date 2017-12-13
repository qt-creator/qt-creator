/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangsupport_global.h"

#include <filepathid.h>
#include <filepathcache.h>

#include <QTimer>

#include <filepathcachinginterface.h>

#include <functional>

namespace ClangBackEnd {

template <typename Timer>
class ChangedFilePathCompressor
{
public:
    ChangedFilePathCompressor(FilePathCachingInterface &filePathCache)
        : m_filePathCache(filePathCache)
    {
        m_timer.setSingleShot(true);
    }

    virtual ~ChangedFilePathCompressor()
    {
    }

    void addFilePath(const QString &filePath)
    {
        FilePathId filePathId = m_filePathCache.filePathId(FilePath(filePath));

        auto found = std::lower_bound(m_filePaths.begin(), m_filePaths.end(), filePathId);

        if (found == m_filePaths.end() || *found != filePathId)
            m_filePaths.insert(found, filePathId);

        restartTimer();
    }

    FilePathIds takeFilePathIds()
    {
        return std::move(m_filePaths);
    }

    virtual void setCallback(std::function<void(ClangBackEnd::FilePathIds &&)> &&callback)
    {
        QObject::connect(&m_timer,
                         &Timer::timeout,
                         [this, callback=std::move(callback)] { callback(takeFilePathIds()); });
    }

unittest_public:
    virtual void restartTimer()
    {
        m_timer.start(20);
    }

    Timer &timer()
    {
        return m_timer;
    }

private:
    FilePathIds m_filePaths;
    Timer m_timer;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
