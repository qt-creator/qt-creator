// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidSdkManagerTest : public QObject
{
    Q_OBJECT
public:
    AndroidSdkManagerTest(QObject *parent = nullptr);
    ~AndroidSdkManagerTest();

private slots:
    void testAndroidSdkManagerProgressParser_data();
    void testAndroidSdkManagerProgressParser();
};
} // namespace Internal
} // namespace Android
