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

#include "clangsymbolsearcher.h"
#include "symbol.h"

#include <cpptools/searchsymbols.h>

#include <cassert>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

ClangSymbolSearcher::ClangSymbolSearcher(ClangIndexer *indexer, const Parameters &parameters, QSet<QString> fileNames, QObject *parent)
    : CppTools::SymbolSearcher(parent)
    , m_indexer(indexer)
    , m_parameters(parameters)
    , m_fileNames(fileNames)
    , m_future(0)
{
    assert(indexer);
}

ClangSymbolSearcher::~ClangSymbolSearcher()
{
}

void ClangSymbolSearcher::runSearch(QFutureInterface<SearchResultItem> &future)
{
    m_future = &future;
    m_indexer->match(this);
    m_future = 0;
}

void ClangSymbolSearcher::search(const QLinkedList<Symbol> &allSymbols)
{
    QString findString = (m_parameters.flags & Find::FindRegularExpression
                          ? m_parameters.text : QRegExp::escape(m_parameters.text));
    if (m_parameters.flags & Find::FindWholeWords)
        findString = QString::fromLatin1("\\b%1\\b").arg(findString);
    QRegExp matcher(findString, (m_parameters.flags & Find::FindCaseSensitively
                                 ? Qt::CaseSensitive : Qt::CaseInsensitive));

    const int chunkSize = 10;
    QVector<Core::SearchResultItem> resultItems;
    resultItems.reserve(100);

    m_future->setProgressRange(0, allSymbols.size() % chunkSize);
    m_future->setProgressValue(0);

    int symbolNr = 0;
    foreach (const Symbol &s, allSymbols) {
        if (symbolNr % chunkSize == 0) {
            m_future->setProgressValue(symbolNr / chunkSize);
            m_future->reportResults(resultItems);
            resultItems.clear();

            if (m_future->isPaused())
                m_future->waitForResume();
            if (m_future->isCanceled())
                return;
        }
        ++symbolNr;

        CppTools::IndexItem info;

        switch (s.m_kind) {
        case Symbol::Enum:
            if (m_parameters.types & SymbolSearcher::Enums) {
                info.type = CppTools::IndexItem::Enum;
                info.symbolType = QLatin1String("enum");
                break;
            } else {
                continue;
            }
        case Symbol::Class:
            if (m_parameters.types & SymbolSearcher::Classes) {
                info.type = CppTools::IndexItem::Class;
                info.symbolType = QLatin1String("class");
                break;
            } else {
                continue;
            }
        case Symbol::Method:
        case Symbol::Function:
        case Symbol::Constructor:
        case Symbol::Destructor:
            if (m_parameters.types & SymbolSearcher::Functions) {
                info.type = CppTools::IndexItem::Function;
                break;
            } else {
                continue;
            }
        case Symbol::Declaration:
            if (m_parameters.types & SymbolSearcher::Declarations) {
                info.type = CppTools::IndexItem::Declaration;
                break;
            } else {
                continue;
            }

        default: continue;
        }

        if (matcher.indexIn(s.m_name) == -1)
            continue;

        info.symbolName = s.m_name;
        info.fullyQualifiedName = s.m_qualification.split(QLatin1String("::")) << s.m_name;
        info.fileName = s.m_location.fileName();
        info.icon = s.iconForSymbol();
        info.line = s.m_location.line();
        info.column = s.m_location.column() - 1;

        Core::SearchResultItem item;
        item.path << s.m_qualification;
        item.text = s.m_name;
        item.icon = info.icon;
        item.textMarkPos = -1;
        item.textMarkLength = 0;
        item.lineNumber = -1;
        item.userData = qVariantFromValue(info);

        resultItems << item;
    }

    if (!resultItems.isEmpty())
        m_future->reportResults(resultItems);
}
