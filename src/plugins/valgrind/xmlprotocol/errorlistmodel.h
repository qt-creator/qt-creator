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

#ifndef LIBVALGRIND_PROTOCOL_ERRORLISTMODEL_H
#define LIBVALGRIND_PROTOCOL_ERRORLISTMODEL_H

#include <debugger/analyzer/detailederrorview.h>
#include <utils/treemodel.h>

#include <QSharedPointer>

namespace Valgrind {
namespace XmlProtocol {

class Error;
class ErrorListModelPrivate;
class Frame;

class ErrorListModel : public Utils::TreeModel
{
    Q_OBJECT

public:
    enum Role {
        ErrorRole = Debugger::DetailedErrorView::FullTextRole + 1,
    };

    class RelevantFrameFinder
    {
    public:
        virtual ~RelevantFrameFinder() {}
        virtual Frame findRelevant(const Error &error) const = 0;
    };

    explicit ErrorListModel(QObject *parent = 0);
    ~ErrorListModel();

    QSharedPointer<const RelevantFrameFinder> relevantFrameFinder() const;
    void setRelevantFrameFinder(const QSharedPointer<const RelevantFrameFinder> &finder);

public slots:
    void addError(const Valgrind::XmlProtocol::Error &error);

private:
    ErrorListModelPrivate *const d;
};

} // namespace XmlProtocol
} // namespace Valgrind

#endif // LIBVALGRIND_PROTOCOL_ERRORLISTMODEL_H
