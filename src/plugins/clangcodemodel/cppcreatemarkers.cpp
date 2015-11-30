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

#include "clangutils.h"
#include "cppcreatemarkers.h"

#include <cplusplus/CppDocument.h>
#include <utils/executeondestruction.h>
#include <utils/runextensions.h>

#include <QCoreApplication>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QThreadPool>

#include <QDebug>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace CppTools;

static Q_LOGGING_CATEGORY(log, "qtc.clangcodemodel.createmarkers")

CreateMarkers *CreateMarkers::create(SemanticMarker::Ptr semanticMarker,
                                     const QString &fileName,
                                     unsigned firstLine, unsigned lastLine)
{
    if (semanticMarker.isNull())
        return 0;
    else
        return new CreateMarkers(semanticMarker, fileName, firstLine, lastLine);
}

CreateMarkers::CreateMarkers(SemanticMarker::Ptr semanticMarker,
                             const QString &fileName,
                             unsigned firstLine, unsigned lastLine)
    : m_marker(semanticMarker)
    , m_fileName(fileName)
    , m_firstLine(firstLine)
    , m_lastLine(lastLine)
{
    Q_ASSERT(!semanticMarker.isNull());

    m_flushRequested = false;
    m_flushLine = 0;
}

CreateMarkers::~CreateMarkers()
{ }

void CreateMarkers::run()
{
    QMutexLocker lock(m_marker->mutex());

    ::Utils::ExecuteOnDestruction reportFinishedOnDestruction([this]() { reportFinished(); });

    if (isCanceled())
        return;

    qCDebug(log) << "Creating markers from" << m_firstLine << "to" << m_lastLine
                 << "of" << m_fileName;
    QTime t; t.start();

    m_usages.clear();

    if (isCanceled())
        return;

    const QList<ClangCodeModel::SourceMarker> markers
        = m_marker->sourceMarkersInRange(m_firstLine, m_lastLine);
    foreach (const ClangCodeModel::SourceMarker &m, markers)
        addUse(SourceMarker(m.location().line(), m.location().column(), m.length(), m.kind()));

    if (isCanceled())
        return;

    flush();

    qCDebug(log) << "Creating markers took" << t.elapsed() << "ms in total.";
}

void CreateMarkers::addUse(const SourceMarker &marker)
{
//    if (! enclosingFunctionDefinition()) {
        if (m_usages.size() >= 100) {
            if (m_flushRequested && marker.line != m_flushLine)
                flush();
            else if (! m_flushRequested) {
                m_flushRequested = true;
                m_flushLine = marker.line;
            }
        }
//    }

    m_usages.append(marker);
}

void CreateMarkers::flush()
{
    m_flushRequested = false;
    m_flushLine = 0;

    if (m_usages.isEmpty())
        return;

    reportResults(m_usages);
    m_usages.clear();
}
