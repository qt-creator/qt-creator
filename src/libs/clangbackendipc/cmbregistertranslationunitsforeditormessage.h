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

#ifndef CLANGBACKEND_REGISTERFILEFOREDITOR_H
#define CLANGBACKEND_REGISTERFILEFOREDITOR_H

#include "filecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class CMBIPC_EXPORT RegisterTranslationUnitForEditorMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const RegisterTranslationUnitForEditorMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, RegisterTranslationUnitForEditorMessage &message);
    friend CMBIPC_EXPORT bool operator==(const RegisterTranslationUnitForEditorMessage &first, const RegisterTranslationUnitForEditorMessage &second);
    friend void PrintTo(const RegisterTranslationUnitForEditorMessage &message, ::std::ostream* os);
public:
    RegisterTranslationUnitForEditorMessage() = default;
    RegisterTranslationUnitForEditorMessage(const QVector<FileContainer> &fileContainers,
                                            const Utf8String &currentEditorFilePath,
                                            const Utf8StringVector &visibleEditorFilePaths);

    const QVector<FileContainer> &fileContainers() const;
    const Utf8String &currentEditorFilePath() const;
    const Utf8StringVector &visibleEditorFilePaths() const;

private:
    QVector<FileContainer> fileContainers_;
    Utf8String currentEditorFilePath_;
    Utf8StringVector visibleEditorFilePaths_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const RegisterTranslationUnitForEditorMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, RegisterTranslationUnitForEditorMessage &message);
CMBIPC_EXPORT bool operator==(const RegisterTranslationUnitForEditorMessage &first, const RegisterTranslationUnitForEditorMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const RegisterTranslationUnitForEditorMessage &message);
void PrintTo(const RegisterTranslationUnitForEditorMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(RegisterTranslationUnitForEditorMessage);
} // namespace ClangBackEnd

#endif // CLANGBACKEND_REGISTERFILEFORCODECOMPLITION_H
