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

#ifndef CLANGBACKEND_DIAGNOSTICSCHANGEDMESSAGE_H
#define CLANGBACKEND_DIAGNOSTICSCHANGEDMESSAGE_H

#include "clangbackendipc_global.h"
#include "diagnosticcontainer.h"
#include "filecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class CMBIPC_EXPORT DiagnosticsChangedMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const DiagnosticsChangedMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, DiagnosticsChangedMessage &message);
    friend CMBIPC_EXPORT bool operator==(const DiagnosticsChangedMessage &first, const DiagnosticsChangedMessage &second);
    friend CMBIPC_EXPORT bool operator<(const DiagnosticsChangedMessage &first, const DiagnosticsChangedMessage &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DiagnosticsChangedMessage &message);
    friend void PrintTo(const DiagnosticsChangedMessage &message, ::std::ostream* os);

public:
    DiagnosticsChangedMessage() = default;
    DiagnosticsChangedMessage(const FileContainer &file,
                              const QVector<DiagnosticContainer> &diagnostics);

    const FileContainer &file() const;
    const QVector<DiagnosticContainer> &diagnostics() const;

private:
    FileContainer file_;
    QVector<DiagnosticContainer> diagnostics_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const DiagnosticsChangedMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, DiagnosticsChangedMessage &message);
CMBIPC_EXPORT bool operator==(const DiagnosticsChangedMessage &first, const DiagnosticsChangedMessage &second);
CMBIPC_EXPORT bool operator<(const DiagnosticsChangedMessage &first, const DiagnosticsChangedMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DiagnosticsChangedMessage &message);
void PrintTo(const DiagnosticsChangedMessage &message, ::std::ostream* os);

} // namespace ClangBackEnd

Q_DECLARE_METATYPE(ClangBackEnd::DiagnosticsChangedMessage)

#endif // CLANGBACKEND_DIAGNOSTICSCHANGEDMESSAGE_H
