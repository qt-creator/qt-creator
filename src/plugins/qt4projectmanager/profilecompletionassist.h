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

#ifndef PROFILECOMPLETIONASSIST_H
#define PROFILECOMPLETIONASSIST_H

#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/codeassist/iassistprocessor.h>

#include <QIcon>

namespace Qt4ProjectManager {
namespace Internal {

class ProFileAssistProposalItem : public TextEditor::BasicProposalItem
{
public:
    ProFileAssistProposalItem();
    virtual ~ProFileAssistProposalItem();

    virtual bool prematurelyApplies(const QChar &c) const;
    virtual void applyContextualContent(TextEditor::BaseTextEditor *editor,
                                        int basePosition) const;
};


class ProFileCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
public:
    ProFileCompletionAssistProvider();
    virtual ~ProFileCompletionAssistProvider();

    virtual bool supportsEditor(const Core::Id &editorId) const;
    virtual TextEditor::IAssistProcessor *createProcessor() const;

    virtual bool isAsynchronous() const;
    virtual int activationCharSequenceLength() const;
    virtual bool isActivationCharSequence(const QString &sequence) const;
    virtual bool isContinuationChar(const QChar &c) const;
};


class ProFileCompletionAssistProcessor : public TextEditor::IAssistProcessor
{
public:
    ProFileCompletionAssistProcessor();
    virtual ~ProFileCompletionAssistProcessor();

    virtual TextEditor::IAssistProposal *perform(const TextEditor::IAssistInterface *interface);

private:
    bool acceptsIdleEditor();
    int findStartOfName(int pos = -1) const;
    bool isInComment() const;

    int m_startPosition;
    QScopedPointer<const TextEditor::IAssistInterface> m_interface;
    const QIcon m_variableIcon;
    const QIcon m_functionIcon;
};


class ProFileAssistProposalModel : public TextEditor::BasicProposalItemListModel
{
public:
    ProFileAssistProposalModel(const QList<TextEditor::BasicProposalItem *> &items);
    virtual ~ProFileAssistProposalModel();

    virtual bool isSortable(const QString &prefix) const;
};

} // Internal
} // Qt4ProjectManager

#endif // PROFILECOMPLETIONASSIST_H
