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

#ifndef IASSISTPROPOSAL_H
#define IASSISTPROPOSAL_H

#include <texteditor/texteditor_global.h>

namespace TextEditor {

class IAssistProposalModel;
class IAssistProposalWidget;
class BaseTextEditor;

class TEXTEDITOR_EXPORT IAssistProposal
{
public:
    IAssistProposal();
    virtual ~IAssistProposal();

    virtual bool isFragile() const = 0;
    virtual int basePosition() const = 0;
    virtual bool isCorrective() const = 0;
    virtual void makeCorrection(BaseTextEditor *editor) = 0;
    virtual IAssistProposalModel *model() const = 0;
    virtual IAssistProposalWidget *createWidget() const = 0;
};

} // TextEditor

#endif // IASSISTPROPOSAL_H
