/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPFINDREFERENCES_H
#define CPPFINDREFERENCES_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <utils/filesearch.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/FindUsages.h>

namespace Find {
    class SearchResultWindow;
    struct SearchResultItem;
} // end of namespace Find

namespace CppTools {
class CppModelManagerInterface;

namespace Internal {

class CppFindReferences: public QObject
{
    Q_OBJECT

public:
    CppFindReferences(CppModelManagerInterface *modelManager);
    virtual ~CppFindReferences();

    QList<int> references(CPlusPlus::Symbol *symbol,
                          CPlusPlus::Document::Ptr doc,
                          const CPlusPlus::Snapshot& snapshot) const;

Q_SIGNALS:
    void changed();

public:
    void findUsages(CPlusPlus::Document::Ptr symbolDocument,CPlusPlus::Symbol *symbol);
    void renameUsages(CPlusPlus::Document::Ptr symbolDocument,CPlusPlus::Symbol *symbol);

    void findMacroUses(const CPlusPlus::Macro &macro);

private Q_SLOTS:
    void displayResults(int first, int last);
    void searchFinished();
    void openEditor(const Find::SearchResultItem &item);
    void onReplaceButtonClicked(const QString &text, const QList<Find::SearchResultItem> &items);

private:
    void findAll_helper(CPlusPlus::Document::Ptr symbolDocument, CPlusPlus::Symbol *symbol);

private:
    QPointer<CppModelManagerInterface> _modelManager;
    Find::SearchResultWindow *_resultWindow;
    QFutureWatcher<CPlusPlus::Usage> m_watcher;
};

} // end of namespace Internal
} // end of namespace CppTools

#endif // CPPFINDREFERENCES_H
