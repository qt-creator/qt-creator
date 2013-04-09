/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPFINDREFERENCES_H
#define CPPFINDREFERENCES_H

#include <cplusplus/DependencyTable.h>
#include <cplusplus/FindUsages.h>

#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QFuture>
#include <QFutureWatcher>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Find {
    struct SearchResultItem;
    class SearchResult;
} // namespace Find

namespace CppTools {
class CppModelManagerInterface;

namespace Internal {

class CppFindReferencesParameters
{
public:
    CPlusPlus::LookupContext context;
    CPlusPlus::Symbol *symbol;
};

class CppFindReferences: public QObject
{
    Q_OBJECT

public:
    CppFindReferences(CppModelManagerInterface *modelManager);
    virtual ~CppFindReferences();

    QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) const;

public:
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);
    void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                      const QString &replacement = QString());

    void findMacroUses(const CPlusPlus::Macro &macro);
    void renameMacroUses(const CPlusPlus::Macro &macro, const QString &replacement = QString());

    CPlusPlus::DependencyTable updateDependencyTable(CPlusPlus::Snapshot snapshot);

private Q_SLOTS:
    void displayResults(int first, int last);
    void searchFinished();
    void cancel();
    void setPaused(bool paused);
    void openEditor(const Find::SearchResultItem &item);
    void onReplaceButtonClicked(const QString &text, const QList<Find::SearchResultItem> &items, bool preserveCase);
    void searchAgain();

private:
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                    const QString &replacement, bool replace);
    void findMacroUses(const CPlusPlus::Macro &macro, const QString &replacement,
                       bool replace);
    void findAll_helper(Find::SearchResult *search);
    CPlusPlus::DependencyTable dependencyTable() const;
    void setDependencyTable(const CPlusPlus::DependencyTable &newTable);
    void createWatcher(const QFuture<CPlusPlus::Usage> &future, Find::SearchResult *search);
    bool findSymbol(CppFindReferencesParameters *parameters,
                    const CPlusPlus::Snapshot &snapshot);

private:
    QPointer<CppModelManagerInterface> _modelManager;
    QMap<QFutureWatcher<CPlusPlus::Usage> *, QPointer<Find::SearchResult> > m_watchers;

    mutable QMutex m_depsLock;
    CPlusPlus::DependencyTable m_deps;
};

} // namespace Internal
} // namespace CppTools

Q_DECLARE_METATYPE(CppTools::Internal::CppFindReferencesParameters)

#endif // CPPFINDREFERENCES_H
