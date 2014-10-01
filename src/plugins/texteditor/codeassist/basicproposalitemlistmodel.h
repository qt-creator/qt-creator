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

#ifndef BASICPROPOSALITEMLISTMODEL_H
#define BASICPROPOSALITEMLISTMODEL_H

#include "igenericproposalmodel.h"

#include <texteditor/texteditor_global.h>

#include <utils/qtcoverride.h>

#include <QList>
#include <QHash>
#include <QPair>

namespace TextEditor {

class BasicProposalItem;

class TEXTEDITOR_EXPORT BasicProposalItemListModel : public IGenericProposalModel
{
public:
    BasicProposalItemListModel();
    BasicProposalItemListModel(const QList<BasicProposalItem *> &items);
    ~BasicProposalItemListModel();

    void reset() QTC_OVERRIDE;
    int size() const QTC_OVERRIDE;
    QString text(int index) const QTC_OVERRIDE;
    QIcon icon(int index) const QTC_OVERRIDE;
    QString detail(int index) const QTC_OVERRIDE;
    int persistentId(int index) const QTC_OVERRIDE;
    void removeDuplicates() QTC_OVERRIDE;
    void filter(const QString &prefix) QTC_OVERRIDE;
    bool isSortable(const QString &prefix) const QTC_OVERRIDE;
    void sort(const QString &prefix) QTC_OVERRIDE;
    bool supportsPrefixExpansion() const QTC_OVERRIDE;
    QString proposalPrefix() const QTC_OVERRIDE;
    bool keepPerfectMatch(AssistReason reason) const QTC_OVERRIDE;
    IAssistProposalItem *proposalItem(int index) const QTC_OVERRIDE;

    void loadContent(const QList<BasicProposalItem *> &items);
    void setSortingAllowed(bool isAllowed);
    bool isSortingAllowed() const;

protected:
    typedef QList<BasicProposalItem *>::iterator ItemIterator;
    QPair<ItemIterator, ItemIterator> currentItems();
    QList<BasicProposalItem *> m_currentItems;

private:
    void mapPersistentIds();

    QHash<QString, int> m_idByText;
    QList<BasicProposalItem *> m_originalItems;
};

} // TextEditor

#endif // BASICPROPOSALITEMLISTMODEL_H
