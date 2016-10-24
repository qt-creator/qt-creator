/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <cplusplus/FindUsages.h>

#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QFuture>
#include <QFutureWatcher>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Core {
class SearchResultItem;
class SearchResult;
} // namespace Core

namespace CppTools {
class CppModelManager;

namespace Internal {

class CppFindReferencesParameters
{
public:
    QList<QByteArray> symbolId;
    QByteArray symbolFileName;
};

class CppFindReferences: public QObject
{
    Q_OBJECT

public:
    CppFindReferences(CppModelManager *modelManager);
    virtual ~CppFindReferences();

    QList<int> references(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context) const;

public:
    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context);
    void renameUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                      const QString &replacement = QString());

    void findMacroUses(const CPlusPlus::Macro &macro);
    void renameMacroUses(const CPlusPlus::Macro &macro, const QString &replacement = QString());

private:
    void openEditor(const Core::SearchResultItem &item);
    void onReplaceButtonClicked(const QString &text, const QList<Core::SearchResultItem> &items, bool preserveCase);
    void searchAgain();

    void findUsages(CPlusPlus::Symbol *symbol, const CPlusPlus::LookupContext &context,
                    const QString &replacement, bool replace);
    void findMacroUses(const CPlusPlus::Macro &macro, const QString &replacement,
                       bool replace);
    void findAll_helper(Core::SearchResult *search, CPlusPlus::Symbol *symbol,
                        const CPlusPlus::LookupContext &context);
    void createWatcher(const QFuture<CPlusPlus::Usage> &future, Core::SearchResult *search);
    CPlusPlus::Symbol *findSymbol(const CppFindReferencesParameters &parameters,
                    const CPlusPlus::Snapshot &snapshot, CPlusPlus::LookupContext *context);

private:
    QPointer<CppModelManager> m_modelManager;
};

} // namespace Internal
} // namespace CppTools

Q_DECLARE_METATYPE(CppTools::Internal::CppFindReferencesParameters)
