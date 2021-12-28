/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "googletest.h"

#include <imagecachestorageinterface.h>

class MockImageCacheStorage : public QmlDesigner::ImageCacheStorageInterface
{
public:
    MOCK_METHOD(QmlDesigner::ImageCacheStorageInterface::ImageEntry,
                fetchImage,
                (Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp),
                (const, override));

    MOCK_METHOD(QmlDesigner::ImageCacheStorageInterface::ImageEntry,
                fetchSmallImage,
                (Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp),
                (const, override));

    MOCK_METHOD(QmlDesigner::ImageCacheStorageInterface::IconEntry,
                fetchIcon,
                (Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp),
                (const, override));

    MOCK_METHOD(void,
                storeImage,
                (Utils::SmallStringView name,
                 Sqlite::TimeStamp newTimeStamp,
                 const QImage &image,
                 const QImage &smallImage),
                (override));

    MOCK_METHOD(void,
                storeIcon,
                (Utils::SmallStringView name, Sqlite::TimeStamp newTimeStamp, const QIcon &icon),
                (override));

    MOCK_METHOD(void, walCheckpointFull, (), (override));
    MOCK_METHOD(Sqlite::TimeStamp,
                fetchModifiedImageTime,
                (Utils::SmallStringView name),
                (const, override));
};
