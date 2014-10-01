/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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

#include <QSet>
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
                                 const QString &fileName,
                                 const QStringList &options,
                                 unsigned firstLine, unsigned lastLine,
                                 Internal::FastIndexer *fastIndexer,
                                 const Internal::PchInfo::Ptr &pchInfo);

    void addUse(const SourceMarker &marker);
    void flush();

protected:
    CreateMarkers(ClangCodeModel::SemanticMarker::Ptr semanticMarker,
                  const QString &fileName, const QStringList &options,
                  unsigned firstLine, unsigned lastLine,
                  Internal::FastIndexer *fastIndexer,
                  const Internal::PchInfo::Ptr &pchInfo);

private:
    ClangCodeModel::SemanticMarker::Ptr m_marker;
    Internal::PchInfo::Ptr m_pchInfo;
    QString m_fileName;
    QStringList m_options;
    unsigned m_firstLine;
    unsigned m_lastLine;
    Internal::FastIndexer *m_fastIndexer;
    QVector<SourceMarker> m_usages;
    bool m_flushRequested;
    unsigned m_flushLine;

    ClangCodeModel::Internal::UnsavedFiles m_unsavedFiles;
};

} // namespace ClangCodeModel

#endif // CPPCREATEMARKERS_H
