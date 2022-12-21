#pragma once


// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

