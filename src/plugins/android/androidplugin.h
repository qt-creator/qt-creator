// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Android::Internal {

class AndroidTests final : public QObject
{
#ifdef WITH_TESTS

    Q_OBJECT

private slots:
   void testAndroidConfigAvailableNdkPlatforms_data();
   void testAndroidConfigAvailableNdkPlatforms();
   void testAndroidQtVersionParseBuiltWith_data();
   void testAndroidQtVersionParseBuiltWith();
#endif // WITH_TESTS
};

} // Android::Internal
