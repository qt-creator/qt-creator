/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "cpplocatorfilter.h"
#include "cppmodelmanager.h"

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>
#include <utils/fileutils.h>

#include <QStringMatcher>

using namespace CppTools::Internal;
using namespace Utils;

CppLocatorFilter::CppLocatorFilter(CppModelManager *manager)
    : m_manager(manager),
    m_forceNewSearchList(true)
{
    setShortcutString(QString(QLatin1Char(':')));
    setIncludedByDefault(false);

    connect(manager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    connect(manager, SIGNAL(aboutToRemoveFiles(QStringList)),
            this, SLOT(onAboutToRemoveFiles(QStringList)));
}

CppLocatorFilter::~CppLocatorFilter()
{ }

void CppLocatorFilter::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    m_searchList[doc->fileName()] = search(doc);
}

void CppLocatorFilter::onAboutToRemoveFiles(const QStringList &files)
{
    foreach (const QString &file, files)
        m_searchList.remove(file);
}

void CppLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

static bool compareLexigraphically(const Locator::FilterEntry &a,
                                   const Locator::FilterEntry &b)
{
    return a.displayName < b.displayName;
}

QList<Locator::FilterEntry> CppLocatorFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<Locator::FilterEntry> goodEntries;
    QList<Locator::FilterEntry> betterEntries;
    const QChar asterisk = QLatin1Char('*');
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    QRegExp regexp(asterisk + entry+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = (entry.contains(asterisk) || entry.contains(QLatin1Char('?')));

    QHashIterator<QString, QList<ModelItemInfo> > it(m_searchList);
    while (it.hasNext()) {
        if (future.isCanceled())
            break;

        it.next();

        const QList<ModelItemInfo> items = it.value();
        foreach (const ModelItemInfo &info, items) {
            if ((hasWildcard && regexp.exactMatch(info.symbolName))
                    || (!hasWildcard && matcher.indexIn(info.symbolName) != -1)) {

                QVariant id = qVariantFromValue(info);
                Locator::FilterEntry filterEntry(this, info.symbolName, id, info.icon);
                if (! info.symbolType.isEmpty()) {
                    filterEntry.extraInfo = info.symbolType;
                } else {
                    filterEntry.extraInfo = FileUtils::shortNativePath(
                        FileName::fromString(info.fileName));
                }

                if (info.symbolName.startsWith(entry))
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
    }

    if (goodEntries.size() < 1000)
        qSort(goodEntries.begin(), goodEntries.end(), compareLexigraphically);
    if (betterEntries.size() < 1000)
        qSort(betterEntries.begin(), betterEntries.end(), compareLexigraphically);

    betterEntries += goodEntries;
    return betterEntries;
}

void CppLocatorFilter::accept(Locator::FilterEntry selection) const
{
    ModelItemInfo info = qvariant_cast<CppTools::ModelItemInfo>(selection.internalData);
    TextEditor::BaseTextEditorWidget::openEditorAt(info.fileName, info.line, info.column,
                                             Core::Id(), Core::EditorManager::ModeSwitch);
}

void CppLocatorFilter::reset()
{
    m_searchList.clear();
    m_previousResults.clear();
    m_previousEntry.clear();
    m_forceNewSearchList = true;
}
