/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "expanddata.h"

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ExpandData::ExpandData(const QString &path_, const QString &displayName_) :
    path(path_), displayName(displayName_)
{ }

bool ExpandData::operator==(const ExpandData &other) const
{
    return path == other.path && displayName == other.displayName;
}

ExpandData ExpandData::fromSettings(const QVariant &v)
{
    QStringList list = v.toStringList();
    return list.size() == 2 ? ExpandData(list.at(0), list.at(1)) : ExpandData();
}

QVariant ExpandData::toSettings() const
{
    return QVariant::fromValue(QStringList({ path, displayName }));
}

int ProjectExplorer::Internal::qHash(const ExpandData &data)
{
    return qHash(data.path) ^ qHash(data.displayName);
}
