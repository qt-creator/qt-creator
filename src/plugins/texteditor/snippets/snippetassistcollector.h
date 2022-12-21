// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor_global.h>

#include <QString>
#include <QList>
#include <QIcon>

namespace TextEditor {

class AssistProposalItemInterface;

class TEXTEDITOR_EXPORT SnippetAssistCollector
{
public:
    SnippetAssistCollector(const QString &groupId, const QIcon &icon, int order = 0);

    void setGroupId(const QString &gid);
    QString groupId() const;

    QList<AssistProposalItemInterface *> collect() const;

private:
    QString m_groupId;
    QIcon m_icon;
    int m_order;
};

} // TextEditor
