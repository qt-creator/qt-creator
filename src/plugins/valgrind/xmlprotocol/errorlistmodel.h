/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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

#include <debugger/analyzer/detailederrorview.h>
#include <utils/treemodel.h>

#include <functional>

namespace Valgrind {
namespace XmlProtocol {

class Error;
class Frame;

class ErrorListModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    enum Role {
        ErrorRole = Debugger::DetailedErrorView::FullTextRole + 1,
    };

    explicit ErrorListModel(QObject *parent = 0);

    typedef std::function<Frame(const Error &)> RelevantFrameFinder;
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

} // namespace XmlProtocol
} // namespace Valgrind
