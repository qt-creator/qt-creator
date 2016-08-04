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

#include <QString>
#include <QDebug>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <gtest/gtest-printers.h>

#include <iostream>

#pragma once

QT_BEGIN_NAMESPACE

class QVariant;
inline void PrintTo(const QVariant &variant, ::std::ostream *os)
{
    QString output;
    QDebug debug(&output);

    debug << variant;

    *os << output.toUtf8().constData();
}

inline void PrintTo(const QString &text, ::std::ostream *os)
{
    *os << text.toUtf8().constData();
}

QT_END_NAMESPACE

namespace clang {

inline void PrintTo(const clang::FullSourceLoc &sourceLocation, ::std::ostream *os)
{
    auto &&sourceManager = sourceLocation.getManager();
    auto fileName = sourceManager.getFileEntryForID(sourceLocation.getFileID())->getName();

    *os << "SourceLocation(\""
        << fileName << ", "
        << sourceLocation.getSpellingLineNumber() << ", "
        << sourceLocation.getSpellingColumnNumber() << ")";
}

}

//namespace testing {
//namespace internal {

// void PrintTo(const QVariant &variant, ::std::ostream *os);

//}
//}
