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

#include "filecontainer.h"

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class RegisterTranslationUnitForEditorMessage
{
public:
    RegisterTranslationUnitForEditorMessage() = default;
    RegisterTranslationUnitForEditorMessage(const QVector<FileContainer> &fileContainers,
                                            const Utf8String &currentEditorFilePath,
                                            const Utf8StringVector &visibleEditorFilePaths)
        : fileContainers_(fileContainers),
          currentEditorFilePath_(currentEditorFilePath),
          visibleEditorFilePaths_(visibleEditorFilePaths)
    {
    }

    const QVector<FileContainer> &fileContainers() const
    {
        return fileContainers_;
    }

    const Utf8String &currentEditorFilePath() const
    {
        return currentEditorFilePath_;
    }

    const Utf8StringVector &visibleEditorFilePaths() const
    {
        return visibleEditorFilePaths_;
    }

    friend QDataStream &operator<<(QDataStream &out, const RegisterTranslationUnitForEditorMessage &message)
    {
        out << message.fileContainers_;
        out << message.currentEditorFilePath_;
        out << message.visibleEditorFilePaths_;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RegisterTranslationUnitForEditorMessage &message)
    {
        in >> message.fileContainers_;
        in >> message.currentEditorFilePath_;
        in >> message.visibleEditorFilePaths_;

        return in;
    }

    friend bool operator==(const RegisterTranslationUnitForEditorMessage &first, const RegisterTranslationUnitForEditorMessage &second)
    {
        return first.fileContainers_ == second.fileContainers_
            && first.currentEditorFilePath_ == second.currentEditorFilePath_
            && first.visibleEditorFilePaths_ == second.visibleEditorFilePaths_;
    }

    friend std::ostream &operator<<(std::ostream &os, const RegisterTranslationUnitForEditorMessage &message);
private:
    QVector<FileContainer> fileContainers_;
    Utf8String currentEditorFilePath_;
    Utf8StringVector visibleEditorFilePaths_;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RegisterTranslationUnitForEditorMessage &message);

DECLARE_MESSAGE(RegisterTranslationUnitForEditorMessage);
} // namespace ClangBackEnd
