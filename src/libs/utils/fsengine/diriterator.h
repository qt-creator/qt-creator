/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "../filepath.h"
#include "../stringutils.h"

#include <QFileInfo>
#include <QString>

#include <QtCore/private/qabstractfileengine_p.h>

namespace Utils {
namespace Internal {

class DirIterator : public QAbstractFileEngineIterator
{
public:
    DirIterator(FilePaths paths)
        : QAbstractFileEngineIterator({}, {})
        , m_filePaths(std::move(paths))
        , it(m_filePaths.begin())
    {}

    // QAbstractFileEngineIterator interface
public:
    QString next() override
    {
        if (it == m_filePaths.end())
            return QString();
        const QString r = currentFilePath();
        ++it;
        return r;
    }
    bool hasNext() const override { return !m_filePaths.empty() && m_filePaths.end() != it + 1; }
    QString currentFileName() const override { return chopIfEndsWith(it->toString(), '/'); }

    QFileInfo currentFileInfo() const override
    {
        return QFileInfo(chopIfEndsWith(it->toString(), '/'));
    }

private:
    const FilePaths m_filePaths;
    FilePaths::const_iterator it;
};

} // namespace Internal

} // namespace Utils
