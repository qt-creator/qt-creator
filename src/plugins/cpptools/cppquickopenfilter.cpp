/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cppquickopenfilter.h"
#include "cppmodelmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

using namespace CppTools::Internal;

CppQuickOpenFilter::CppQuickOpenFilter(CppModelManager *manager, Core::EditorManager *editorManager)
    : m_manager(manager),
    m_editorManager(editorManager),
    m_forceNewSearchList(true)
{
    setShortcutString(":");
    setIncludedByDefault(false);

    connect(manager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    connect(manager, SIGNAL(aboutToRemoveFiles(QStringList)),
            this, SLOT(onAboutToRemoveFiles(QStringList)));
}

CppQuickOpenFilter::~CppQuickOpenFilter()
{ }

void CppQuickOpenFilter::onDocumentUpdated(CPlusPlus::Document::Ptr doc)
{
    m_searchList[doc->fileName()] = Info(doc);
}

void CppQuickOpenFilter::onAboutToRemoveFiles(const QStringList &files)
{
    foreach (const QString &file, files)
        m_searchList.remove(file);
}

void CppQuickOpenFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future);
}

static bool compareLexigraphically(const QuickOpen::FilterEntry &a,
                                   const QuickOpen::FilterEntry &b)
{
    return a.displayName < b.displayName;
}

QList<QuickOpen::FilterEntry> CppQuickOpenFilter::matchesFor(const QString &origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<QuickOpen::FilterEntry> entries;
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    const QRegExp regexp("*"+entry+"*", Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return entries;
    bool hasWildcard = (entry.contains('*') || entry.contains('?'));

    QMutableMapIterator<QString, Info> it(m_searchList);
    while (it.hasNext()) {
        it.next();

        Info info = it.value();
        if (info.dirty) {
            info.dirty = false;
            info.items = search(info.doc);
            it.setValue(info);
        }

        QList<ModelItemInfo> items = info.items;

        foreach (ModelItemInfo info, items) {
            if ((hasWildcard && regexp.exactMatch(info.symbolName))
                    || (!hasWildcard && matcher.indexIn(info.symbolName) != -1)) {
                QVariant id = qVariantFromValue(info);
                QuickOpen::FilterEntry filterEntry(this, info.symbolName, id, info.icon);
                filterEntry.extraInfo = info.symbolType;
                entries.append(filterEntry);
            }
        }
    }

    if (entries.size() < 1000)
        qSort(entries.begin(), entries.end(), compareLexigraphically);

    return entries;
}

void CppQuickOpenFilter::accept(QuickOpen::FilterEntry selection) const
{
    ModelItemInfo info = qvariant_cast<CppTools::Internal::ModelItemInfo>(selection.internalData);
    TextEditor::BaseTextEditor::openEditorAt(info.fileName, info.line);
}
