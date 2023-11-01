// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/analyzer/detailederrorview.h>
#include <utils/treemodel.h>

#include <functional>

namespace Valgrind::XmlProtocol {

class Error;
class Frame;

class ErrorListModel : public Utils::TreeModel<>
{
public:
    enum Role {
        ErrorRole = Debugger::DetailedErrorView::FullTextRole + 1,
    };

    explicit ErrorListModel(QObject *parent = nullptr);

    using RelevantFrameFinder = std::function<Frame (const Error &)>;
    RelevantFrameFinder relevantFrameFinder() const;
    void setRelevantFrameFinder(const RelevantFrameFinder &relevantFrameFinder);

    void addError(const Valgrind::XmlProtocol::Error &error);

private:
    friend class ErrorItem;
    friend class StackItem;
    QVariant errorData(const QModelIndex &index, int role) const;
    Frame findRelevantFrame(const Error &error) const;
    QString errorLocation(const Error &error) const;

    RelevantFrameFinder m_relevantFrameFinder;
};

} // namespace Valgrind::XmlProtocol
