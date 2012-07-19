/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef IGENERICPROPOSALMODEL_H
#define IGENERICPROPOSALMODEL_H

#include "iassistproposalmodel.h"
#include "assistenums.h"

#include <texteditor/texteditor_global.h>

#include <QIcon>

namespace TextEditor {

class IAssistProposalItem;

class TEXTEDITOR_EXPORT IGenericProposalModel : public IAssistProposalModel
{
public:
    IGenericProposalModel();
    virtual ~IGenericProposalModel();

    virtual QIcon icon(int index) const = 0;
    virtual QString detail(int index) const = 0;
    virtual int persistentId(int index) const = 0;
    virtual void removeDuplicates() = 0;
    virtual void filter(const QString &prefix) = 0;
    virtual bool isSortable(const QString &prefix) const = 0;
    virtual void sort() = 0;
    virtual bool supportsPrefixExpansion() const = 0;
    virtual QString proposalPrefix() const = 0;
    virtual bool keepPerfectMatch(AssistReason reason) const = 0;
    virtual IAssistProposalItem *proposalItem(int index) const = 0;
};

} // TextEditor

#endif // IGENERICPROPOSALMODEL_H
