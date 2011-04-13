/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CPPFINDREFERENCES_H
#define CPPFINDREFERENCES_H

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <utils/filesearch.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/DependencyTable.h>
#include <cplusplus/FindUsages.h>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Find {
    class SearchResultWindow;
    struct SearchResultItem;
} // namespace Find

namespace CPlusPlus {
class CppModelManagerInterface;
}

namespace CppTools {
namespace Internal {

class CppFindReferences: public QObject
{
    Q_OBJECT

public:
    CppFindReferences(CPlusPlus::CppModelManagerInterface *modelManager);
    virtual ~CppFindReferences();

    QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) const;

Q_SIGNALS:
    void changed();

public:
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);
    void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                      const QString &replacement = QString());

    void findMacroUses(const CPlusPlus::Macro &macro);

    CPlusPlus::DependencyTable updateDependencyTable(CPlusPlus::Snapshot snapshot);

private Q_SLOTS:
    void displayResults(int first, int last);
    void searchFinished();
    void openEditor(const Find::SearchResultItem &item);
    void onReplaceButtonClicked(const QString &text, const QList<Find::SearchResultItem> &items);

private:
    void findAll_helper(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);
    CPlusPlus::DependencyTable dependencyTable() const;
    void setDependencyTable(const CPlusPlus::DependencyTable &newTable);

private:
    QPointer<CPlusPlus::CppModelManagerInterface> _modelManager;
    Find::SearchResultWindow *_resultWindow;
    QFutureWatcher<CPlusPlus::Usage> m_watcher;

    mutable QMutex m_depsLock;
    CPlusPlus::DependencyTable m_deps;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPFINDREFERENCES_H
