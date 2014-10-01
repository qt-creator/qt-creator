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

#include "clanghighlightingsupport.h"

#include <texteditor/basetextdocument.h>

#include <QTextBlock>
#include <QTextEdit>
#include "pchmanager.h"

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace CppTools;

ClangHighlightingSupport::ClangHighlightingSupport(TextEditor::BaseTextDocument *baseTextDocument,
                                                   FastIndexer *fastIndexer)
    : CppHighlightingSupport(baseTextDocument)
    , m_fastIndexer(fastIndexer)
    , m_semanticMarker(new ClangCodeModel::SemanticMarker)
{
}

ClangHighlightingSupport::~ClangHighlightingSupport()
{
}

bool ClangHighlightingSupport::hightlighterHandlesIfdefedOutBlocks() const
{
#if CINDEX_VERSION_MINOR >= 21
    return true;
#else
    return false;
#endif
}

QFuture<TextEditor::HighlightingResult> ClangHighlightingSupport::highlightingFuture(
        const CPlusPlus::Document::Ptr &doc,
        const CPlusPlus::Snapshot &snapshot) const
{
    Q_UNUSED(doc);
    Q_UNUSED(snapshot);

    int firstLine = 1;
    int lastLine = baseTextDocument()->document()->blockCount();

    const QString fileName = baseTextDocument()->filePath();
    CppModelManagerInterface *modelManager = CppModelManagerInterface::instance();
    QList<ProjectPart::Ptr> parts = modelManager->projectPart(fileName);
    if (parts.isEmpty())
        parts += modelManager->fallbackProjectPart();
    QStringList options;
    PchInfo::Ptr pchInfo;
    foreach (const ProjectPart::Ptr &part, parts) {
        if (part.isNull())
            continue;
        options = Utils::createClangOptions(part, fileName);
        pchInfo = PchManager::instance()->pchInfo(part);
        if (!pchInfo.isNull())
            options.append(ClangCodeModel::Utils::createPCHInclusionOptions(pchInfo->fileName()));
        if (!options.isEmpty())
            break;
    }

    CreateMarkers *createMarkers = CreateMarkers::create(m_semanticMarker,
                                                         fileName, options,
                                                         firstLine, lastLine,
                                                         m_fastIndexer, pchInfo);
    return createMarkers->start();
}
