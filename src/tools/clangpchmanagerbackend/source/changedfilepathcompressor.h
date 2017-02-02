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

#include <clangpchmanagerbackend_global.h>

#include <utils/smallstringvector.h>

#include <QTimer>

#include <functional>

namespace ClangBackEnd {

template <typename Timer>
class ChangedFilePathCompressor non_unittest_final
{
public:
    ChangedFilePathCompressor()
    {
        m_timer.setSingleShot(true);
    }

    virtual ~ChangedFilePathCompressor()
    {
    }

    void addFilePath(const QString &filePath)
    {
        m_filePaths.push_back(filePath);

        restartTimer();
    }

    Utils::PathStringVector takeFilePaths()
    {
        return std::move(m_filePaths);
    }

    virtual void setCallback(std::function<void(Utils::PathStringVector &&)> &&callback)
    {
        QObject::connect(&m_timer,
                         &Timer::timeout,
                         [this, callback=std::move(callback)] { callback(takeFilePaths()); });
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
    Utils::PathStringVector m_filePaths;
    Timer m_timer;
};

} // namespace ClangBackEnd
