/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
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
