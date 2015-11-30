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

#ifndef CLANG_SEMANTICMARKER_H
#define CLANG_SEMANTICMARKER_H

#include "clang_global.h"
#include "diagnostic.h"
#include "fastindexer.h"
#include "sourcemarker.h"
#include "utils.h"

#include <QMutex>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

namespace ClangCodeModel {

class CLANG_EXPORT SemanticMarker
{
    Q_DISABLE_COPY(SemanticMarker)

public:
    typedef QSharedPointer<SemanticMarker> Ptr;

    class Range
    {
        Range();
    public:
        Range(int first, int last) : first(first), last(last) {}

        int first;
        int last;
    };

public:
    SemanticMarker();
    ~SemanticMarker();

    QMutex *mutex() const
    { return &m_mutex; }

    QString fileName() const;
    void setFileName(const QString &fileName);

    void setCompilationOptions(const QStringList &options);

    void reparse(const Internal::UnsavedFiles &unsavedFiles);

    QList<Diagnostic> diagnostics() const;

    QList<Range> ifdefedOutBlocks() const;

    QList<SourceMarker> sourceMarkersInRange(unsigned firstLine,
                                             unsigned lastLine);

    Internal::Unit::Ptr unit() const;

private:
    mutable QMutex m_mutex;
    Internal::Unit::Ptr m_unit;
};

} // namespace ClangCodeModel

#endif // CLANG_SEMANTICMARKER_H
