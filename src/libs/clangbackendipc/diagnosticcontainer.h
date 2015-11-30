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

#ifndef CLANGBACKEND_DIAGNOSTICCONTAINER_H
#define CLANGBACKEND_DIAGNOSTICCONTAINER_H

#include "sourcerangecontainer.h"
#include "fixitcontainer.h"

#include <QVector>

#include <utility>

namespace ClangBackEnd {

class CMBIPC_EXPORT DiagnosticContainer
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const DiagnosticContainer &container);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, DiagnosticContainer &container);
    friend CMBIPC_EXPORT bool operator==(const DiagnosticContainer &first, const DiagnosticContainer &second);
    friend CMBIPC_EXPORT bool operator<(const DiagnosticContainer &first, const DiagnosticContainer &second);

public:
    DiagnosticContainer() = default;
    DiagnosticContainer(const Utf8String &text,
                        const Utf8String &category,
                        const std::pair<Utf8String,Utf8String> &options,
                        DiagnosticSeverity severity,
                        const SourceLocationContainer &location,
                        const QVector<SourceRangeContainer> &ranges,
                        const QVector<FixItContainer> &fixIts,
                        const QVector<DiagnosticContainer> &children);

    const Utf8String &text() const;
    const Utf8String &category() const;
    const Utf8String &enableOption() const;
    const Utf8String &disableOption() const;
    const SourceLocationContainer &location() const;
    const QVector<SourceRangeContainer> &ranges() const;
    DiagnosticSeverity severity() const;
    const QVector<FixItContainer> &fixIts() const;
    const QVector<DiagnosticContainer> &children() const;

private:
    quint32 &severityAsInt();

private:
    SourceLocationContainer location_;
    QVector<SourceRangeContainer> ranges_;
    Utf8String text_;
    Utf8String category_;
    Utf8String enableOption_;
    Utf8String disableOption_;
    QVector<DiagnosticContainer> children_;
    QVector<FixItContainer> fixIts_;
    DiagnosticSeverity severity_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const DiagnosticContainer &container);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, DiagnosticContainer &container);
CMBIPC_EXPORT bool operator==(const DiagnosticContainer &first, const DiagnosticContainer &second);
CMBIPC_EXPORT bool operator<(const DiagnosticContainer &first, const DiagnosticContainer &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DiagnosticContainer &container);
void PrintTo(const DiagnosticContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd

Q_DECLARE_METATYPE(ClangBackEnd::DiagnosticContainer)

#endif // CLANGBACKEND_DIAGNOSTICCONTAINER_H
