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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangutils.h"
#include "cppcreatemarkers.h"

#include <cplusplus/CppDocument.h>
#include <utils/runextensions.h>

#include <QCoreApplication>
#include <QMutexLocker>
#include <QThreadPool>

#include <QDebug>

static const bool DebugTiming = qgetenv("QTC_CLANG_VERBOSE") == "1";

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace CppTools;

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
    if (isCanceled())
        return;

    QTime t;
    if (DebugTiming) {
        qDebug() << "*** Highlighting from" << m_firstLine << "to" << m_lastLine << "of" << m_fileName;
        t.start();
    }

    m_usages.clear();

    if (isCanceled()) {
        reportFinished();
        return;
    }

    const QList<ClangCodeModel::SourceMarker> markers
        = m_marker->sourceMarkersInRange(m_firstLine, m_lastLine);
    foreach (const ClangCodeModel::SourceMarker &m, markers)
        addUse(SourceMarker(m.location().line(), m.location().column(), m.length(), m.kind()));

    if (isCanceled()) {
        reportFinished();
        return;
    }

    flush();
    reportFinished();

    if (DebugTiming) {
        qDebug() << "*** Highlighting took" << t.elapsed() << "ms in total.";
        t.restart();
    }
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
