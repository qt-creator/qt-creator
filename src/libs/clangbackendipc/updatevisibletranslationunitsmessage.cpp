/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "updatevisibletranslationunitsmessage.h"

#include "container_common.h"

#include <QDataStream>
#include <QDebug>

#include <algorithm>
#include <iterator>
#include <ostream>

namespace ClangBackEnd {

UpdateVisibleTranslationUnitsMessage::UpdateVisibleTranslationUnitsMessage(
        const Utf8String &currentEditorFilePath,
        const Utf8StringVector &visibleEditorFilePaths)
    : currentEditorFilePath_(currentEditorFilePath),
      visibleEditorFilePaths_(visibleEditorFilePaths)
{
}

const Utf8String &UpdateVisibleTranslationUnitsMessage::currentEditorFilePath() const
{
    return currentEditorFilePath_;
}

const Utf8StringVector &UpdateVisibleTranslationUnitsMessage::visibleEditorFilePaths() const
{
    return visibleEditorFilePaths_;
}

QDataStream &operator<<(QDataStream &out, const UpdateVisibleTranslationUnitsMessage &message)
{
    out << message.currentEditorFilePath_;
    out << message.visibleEditorFilePaths_;

    return out;
}

QDataStream &operator>>(QDataStream &in, UpdateVisibleTranslationUnitsMessage &message)
{
    in >> message.currentEditorFilePath_;
    in >> message.visibleEditorFilePaths_;

    return in;
}

bool operator==(const UpdateVisibleTranslationUnitsMessage &first, const UpdateVisibleTranslationUnitsMessage &second)
{
    return first.currentEditorFilePath_ == second.currentEditorFilePath_
        && first.visibleEditorFilePaths_ == second.visibleEditorFilePaths_;
}

bool operator<(const UpdateVisibleTranslationUnitsMessage &first, const UpdateVisibleTranslationUnitsMessage &second)
{
    return first.currentEditorFilePath_ < second.currentEditorFilePath_
        && compareContainer(first.visibleEditorFilePaths_, second.visibleEditorFilePaths_);
}

QDebug operator<<(QDebug debug, const UpdateVisibleTranslationUnitsMessage &message)
{
    debug.nospace() << "UpdateVisibleTranslationUnitsMessage(";

    debug.nospace() << message.currentEditorFilePath()  << ", ";

    for (const Utf8String &visibleEditorFilePath : message.visibleEditorFilePaths())
        debug.nospace() << visibleEditorFilePath << ", ";

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const UpdateVisibleTranslationUnitsMessage &message, ::std::ostream* os)
{
    *os << "UpdateVisibleTranslationUnitsMessage(";

    *os << message.currentEditorFilePath().constData()  << ", ";

    auto visiblePaths = message.visibleEditorFilePaths();

    std::copy(visiblePaths.cbegin(), visiblePaths.cend(), std::ostream_iterator<Utf8String>(*os, ", "));

    *os << ")";
}

} // namespace ClangBackEnd
