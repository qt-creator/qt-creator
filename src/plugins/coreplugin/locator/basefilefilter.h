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

#include "ilocatorfilter.h"

#include <QSharedPointer>
#include <QStringList>

namespace Core {

namespace Internal { class BaseFileFilterPrivate; }

class CORE_EXPORT BaseFileFilter : public ILocatorFilter
{
    Q_OBJECT

public:
    class CORE_EXPORT Iterator {
    public:
        virtual ~Iterator();
        virtual void toFront() = 0;
        virtual bool hasNext() const = 0;
        virtual QString next() = 0;
        virtual QString filePath() const = 0;
        virtual QString fileName() const = 0;
    };

    class CORE_EXPORT ListIterator : public Iterator {
    public:
        ListIterator(const QStringList &filePaths);
        ListIterator(const QStringList &filePaths, const QStringList &fileNames);

        void toFront();
        bool hasNext() const;
        QString next();
        QString filePath() const;
        QString fileName() const;

    private:
        QStringList m_filePaths;
        QStringList m_fileNames;
        QStringList::const_iterator m_pathPosition;
        QStringList::const_iterator m_namePosition;
    };

    BaseFileFilter();
    ~BaseFileFilter();
    void prepareSearch(const QString &entry);
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry);
    void accept(LocatorFilterEntry selection) const;

protected:
    void setFileIterator(Iterator *iterator);
    QSharedPointer<Iterator> fileIterator();

private:
    void updatePreviousResultData();

    Internal::BaseFileFilterPrivate *d;
};

} // namespace Core
