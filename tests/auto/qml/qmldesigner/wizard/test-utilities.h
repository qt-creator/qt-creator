#pragma once


/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "gmock/gmock-matchers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QtGlobal>
#include <QVariant>
#include <QDebug>

#include <utils/algorithm.h>

using ::testing::Return;
using ::testing::AtLeast;
using ::testing::ElementsAreArray;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::SizeIs;

QT_BEGIN_NAMESPACE

inline std::ostream &operator<<(std::ostream &out, const QByteArray &byteArray)
{
    if (byteArray.contains('\n')) {
        QByteArray formattedArray = byteArray;
        formattedArray.replace("\n", "\n\t");
        out << "\n\t";
        out.write(formattedArray.data(), formattedArray.size());
    } else {
        out << "\"";
        out.write(byteArray.data(), byteArray.size());
        out << "\"";
    }

    return out;
}

inline std::ostream &operator<<(std::ostream &out, const QVariant &variant)
{
    QString output;
    QDebug debug(&output);

    debug.noquote().nospace() << variant;

    QByteArray utf8Text = output.toUtf8();

    return out.write(utf8Text.data(), utf8Text.size());
}


inline std::ostream &operator<<(std::ostream &out, const QString &text)
{
    return out << text.toUtf8();
}

inline void PrintTo(const QString &text, std::ostream *os)
{
    *os << text;
}

inline void PrintTo(const QVariant &variant, std::ostream *os)
{
    *os << variant;
}

inline void PrintTo(const QByteArray &text, std::ostream *os)
{
    *os << text;
}

QT_END_NAMESPACE

