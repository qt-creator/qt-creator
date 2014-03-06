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
#include <cpptools/cppmodelmanagerinterface.h>
#include <utils/hostosinfo.h>
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
                                     const QStringList &options,
                                     unsigned firstLine, unsigned lastLine,
                                     FastIndexer *fastIndexer,
                                     const Internal::PchInfo::Ptr &pchInfo)
{
    if (semanticMarker.isNull())
        return 0;
    else
        return new CreateMarkers(semanticMarker, fileName, options, firstLine, lastLine, fastIndexer, pchInfo);
}

CreateMarkers::CreateMarkers(SemanticMarker::Ptr semanticMarker,
                             const QString &fileName,
                             const QStringList &options,
                             unsigned firstLine, unsigned lastLine,
                             FastIndexer *fastIndexer,
                             const Internal::PchInfo::Ptr &pchInfo)
    : m_marker(semanticMarker)
    , m_pchInfo(pchInfo)
    , m_fileName(fileName)
    , m_options(options)
    , m_firstLine(firstLine)
    , m_lastLine(lastLine)
    , m_fastIndexer(fastIndexer)
{
    Q_ASSERT(!semanticMarker.isNull());

    m_flushRequested = false;
    m_flushLine = 0;

    m_unsavedFiles = Utils::createUnsavedFiles(CppModelManagerInterface::instance()->workingCopy());
}

CreateMarkers::~CreateMarkers()
{ }

static QString commandLine(const QStringList &options, const QString &fileName)
{
    const QStringList allOptions = QStringList(options)
        << QLatin1String("-fsyntax-only") << fileName;
    QStringList allOptionsQuoted;
    foreach (const QString &option, allOptions)
        allOptionsQuoted.append(QLatin1Char('\'') + option + QLatin1Char('\''));
    return ::Utils::HostOsInfo::withExecutableSuffix(QLatin1String("clang"))
        + QLatin1Char(' ') + allOptionsQuoted.join(QLatin1String(" "));
}

void CreateMarkers::run()
{
    QMutexLocker lock(m_marker->mutex());
    if (isCanceled())
        return;

    m_options += QLatin1String("-fspell-checking");

    QTime t;
    if (DebugTiming) {
        qDebug() << "*** Highlighting from" << m_firstLine << "to" << m_lastLine << "of" << m_fileName;
        qDebug("***** Options (cmd line equivalent): %s",
               commandLine(m_options, m_fileName).toUtf8().constData());
        t.start();
    }

    m_usages.clear();
    m_marker->setFileName(m_fileName);
    m_marker->setCompilationOptions(m_options);

    m_marker->reparse(m_unsavedFiles);

    if (DebugTiming)
        qDebug() << "*** Reparse for highlighting took" << t.elapsed() << "ms.";

    m_pchInfo.clear();

    typedef CPlusPlus::Document::DiagnosticMessage OtherDiagnostic;
    QList<OtherDiagnostic> msgs;
    foreach (const ClangCodeModel::Diagnostic &d, m_marker->diagnostics()) {
        if (DebugTiming)
            qDebug() << d.severityAsString() << d.location() << d.spelling();

        if (d.location().fileName() != m_marker->fileName())
            continue;

        // TODO: retrieve fix-its for this diagnostic

        int level;
        switch (d.severity()) {
        case Diagnostic::Fatal: level = OtherDiagnostic::Fatal; break;
        case Diagnostic::Error: level = OtherDiagnostic::Error; break;
        case Diagnostic::Warning: level = OtherDiagnostic::Warning; break;
        default: continue;
        }
        msgs.append(OtherDiagnostic(level, d.location().fileName(), d.location().line(),
                                    d.location().column(), d.spelling(), d.length()));
    }
    if (isCanceled()) {
        reportFinished();
        return;
    }

    CppModelManagerInterface *mmi = CppModelManagerInterface::instance();
    static const QString key = QLatin1String("ClangCodeModel.Diagnostics");
    mmi->setExtraDiagnostics(m_marker->fileName(), key, msgs);
#if CINDEX_VERSION_MINOR >= 21
    mmi->setIfdefedOutBlocks(m_marker->fileName(), m_marker->ifdefedOutBlocks());
#endif

    if (isCanceled()) {
        reportFinished();
        return;
    }

    QList<ClangCodeModel::SourceMarker> markers = m_marker->sourceMarkersInRange(m_firstLine, m_lastLine);
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

    if (m_fastIndexer)
        m_fastIndexer->indexNow(m_marker->unit());

    if (DebugTiming)
        qDebug() << "*** Fast re-indexing took" << t.elapsed() << "ms in total.";
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
