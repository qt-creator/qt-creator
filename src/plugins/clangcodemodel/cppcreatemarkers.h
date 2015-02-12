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

#ifndef CPPCREATEMARKERS_H
#define CPPCREATEMARKERS_H

#include "fastindexer.h"
#include "sourcemarker.h"
#include "semanticmarker.h"
#include "pchinfo.h"

#include <texteditor/semantichighlighter.h>

#include <QFuture>
#include <QtConcurrentRun>

namespace ClangCodeModel {

class CreateMarkers:
        public QObject,
        public QRunnable,
        public QFutureInterface<TextEditor::HighlightingResult>
{
    Q_OBJECT
    Q_DISABLE_COPY(CreateMarkers)

public:
    virtual ~CreateMarkers();

    virtual void run();

    typedef TextEditor::HighlightingResult SourceMarker;

    typedef QFuture<SourceMarker> Future;

    Future start()
    {
        this->setRunnable(this);
        this->reportStarted();
        Future future = this->future();
        QThreadPool::globalInstance()->start(this, QThread::LowestPriority);
        return future;
    }

    static CreateMarkers *create(ClangCodeModel::SemanticMarker::Ptr semanticMarker,
                                 const QString &fileName, unsigned firstLine, unsigned lastLine);

    void addUse(const SourceMarker &marker);
    void flush();

protected:
    CreateMarkers(ClangCodeModel::SemanticMarker::Ptr semanticMarker,
                  const QString &fileName, unsigned firstLine, unsigned lastLine);

private:
    ClangCodeModel::SemanticMarker::Ptr m_marker;
    QString m_fileName;
    unsigned m_firstLine;
    unsigned m_lastLine;
    QVector<SourceMarker> m_usages;
    bool m_flushRequested;
    unsigned m_flushLine;
};

} // namespace ClangCodeModel

#endif // CPPCREATEMARKERS_H
