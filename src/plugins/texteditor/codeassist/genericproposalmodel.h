// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "iassistproposalmodel.h"
#include "assistenums.h"

#include <texteditor/completionsettings.h>
#include <texteditor/texteditor_global.h>
#include <utils/fuzzymatcher.h>

#include <QHash>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace TextEditor {

class AssistProposalItemInterface;

class TEXTEDITOR_EXPORT GenericProposalModel : public IAssistProposalModel
{
public:
    GenericProposalModel();
    ~GenericProposalModel() override;

    void reset() override;
    int size() const override;
    QString text(int index) const override;

    virtual QIcon icon(int index) const;
    virtual QString detail(int index) const;
    virtual Qt::TextFormat detailFormat(int index) const;
    virtual int persistentId(int index) const;
    virtual bool containsDuplicates() const;
    virtual void removeDuplicates();
    virtual void filter(const QString &prefix);
    virtual bool isSortable(const QString &prefix) const;
    virtual void sort(const QString &prefix);
    virtual bool supportsPrefixExpansion() const;
    virtual QString proposalPrefix() const;
    virtual bool keepPerfectMatch(AssistReason reason) const;
    virtual AssistProposalItemInterface *proposalItem(int index) const;
    virtual int indexOf(const std::function<bool (AssistProposalItemInterface *)> &predicate) const;

    void loadContent(const QList<AssistProposalItemInterface *> &items);

    bool isPerfectMatch(const QString &prefix) const;
    bool hasItemsToPropose(const QString &prefix, AssistReason reason) const;

    bool isPrefiltered(const QString &prefix) const;
    void setPrefilterPrefix(const QString &prefix);

    FuzzyMatcher::CaseSensitivity convertCaseSensitivity(TextEditor::CaseSensitivity textEditorCaseSensitivity);

protected:
    QList<AssistProposalItemInterface *> m_currentItems;

private:
    QHash<QString, int> m_idByText;
    QList<AssistProposalItemInterface *> m_originalItems;
    QString m_prefilterPrefix;
    bool m_duplicatesRemoved = false;
};

using GenericProposalModelPtr = QSharedPointer<GenericProposalModel>;

} // TextEditor
