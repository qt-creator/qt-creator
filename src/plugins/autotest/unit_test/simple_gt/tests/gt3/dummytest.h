// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <gtest/gtest.h>

#include <QString>

class DummyTest : public ::testing::TestWithParam<const char *>
{
protected:
    virtual void SetUp() {
        m_str1 = QString::fromLatin1(GetParam()).toLower();

        m_str2 = QString::fromLatin1(GetParam()).toUpper();

        m_str3 = QString::fromLatin1(GetParam()).toHtmlEscaped();
    }

    QString m_str1;
    QString m_str2;
    QString m_str3;
};
