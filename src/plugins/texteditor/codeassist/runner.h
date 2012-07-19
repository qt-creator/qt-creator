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

#ifndef PROCESSORRUNNER_H
#define PROCESSORRUNNER_H

#include "iassistproposalwidget.h"

#include <QThread>

namespace TextEditor {

class IAssistProcessor;
class IAssistProposal;
class IAssistInterface;

namespace Internal {

class ProcessorRunner : public QThread
{
    Q_OBJECT

public:
    ProcessorRunner();
    virtual ~ProcessorRunner();

    void setProcessor(IAssistProcessor *processor); // Takes ownership of the processor.
    void setAssistInterface(IAssistInterface *interface);
    void setDiscardProposal(bool discard);

    // @TODO: Not really necessary...
    void setReason(AssistReason reason);
    AssistReason reason() const;

    virtual void run();

    IAssistProposal *proposal() const;

private:
    IAssistProcessor *m_processor;
    IAssistInterface *m_interface;
    bool m_discardProposal;
    IAssistProposal *m_proposal;
    AssistReason m_reason;
};

} // Internal
} // TextEditor

#endif // PROCESSORRUNNER_H
