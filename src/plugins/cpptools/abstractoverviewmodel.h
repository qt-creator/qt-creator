/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "cpptools_global.h"

#include <utils/dropsupport.h>
#include <utils/treemodel.h>

#include <QSharedPointer>

#include <utility>

namespace CPlusPlus { class Document; }

namespace Utils {
class LineColumn;
struct Link;
}

namespace CppTools {

class CPPTOOLS_EXPORT AbstractOverviewModel : public Utils::TreeModel<>
{
    Q_OBJECT

public:
    enum Role {
        FileNameRole = Qt::UserRole + 1,
        LineNumberRole
    };

    virtual void rebuild(QSharedPointer<CPlusPlus::Document>) {}
    virtual bool rebuild(const QString &) { return false; }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    }

    Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    QStringList mimeTypes() const override
    {
        return Utils::DropSupport::mimeTypesForFilePaths();
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        auto mimeData = new Utils::DropMimeData;
        foreach (const QModelIndex &index, indexes) {
            const QVariant fileName = data(index, FileNameRole);
            if (!fileName.canConvert<QString>())
                continue;
            const QVariant lineNumber = data(index, LineNumberRole);
            if (!lineNumber.canConvert<unsigned>())
                continue;
            mimeData->addFile(fileName.toString(), static_cast<int>(lineNumber.value<unsigned>()));
        }
        return mimeData;
    }

    virtual bool isGenerated(const QModelIndex &) const { return false; }
    virtual Utils::Link linkFromIndex(const QModelIndex &) const = 0;
    virtual Utils::LineColumn lineColumnFromIndex(const QModelIndex &) const = 0;

    using Range = std::pair<Utils::LineColumn, Utils::LineColumn>;
    virtual Range rangeFromIndex(const QModelIndex &) const = 0;

signals:
    void needsUpdate();
};

} // namespace CppTools
