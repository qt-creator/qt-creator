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

#include "cppfunctionsfilter.h"

#include <utils/fileutils.h>

using namespace CppTools;
using namespace CppTools::Internal;

CppFunctionsFilter::CppFunctionsFilter(CppLocatorData *locatorData)
    : CppLocatorFilter(locatorData)
{
    setId("Methods");
    setDisplayName(tr("C++ Functions"));
    setShortcutString(QString(QLatin1Char('m')));
    setIncludedByDefault(false);
}

CppFunctionsFilter::~CppFunctionsFilter()
{
}

Core::LocatorFilterEntry CppFunctionsFilter::filterEntryFromIndexItem(IndexItem::Ptr info)
{
    const QVariant id = qVariantFromValue(info);

    QString name = info->symbolName();
    QString extraInfo = info->symbolScope();
    info->unqualifiedNameAndScope(name, &name, &extraInfo);
    if (extraInfo.isEmpty()) {
        extraInfo = info->shortNativeFilePath();
    } else {
        extraInfo.append(QLatin1String(" ("))
                .append(Utils::FileName::fromString(info->fileName()).fileName())
                .append(QLatin1String(")"));
    }

    Core::LocatorFilterEntry filterEntry(this, name + info->symbolType(), id, info->icon());
    filterEntry.extraInfo = extraInfo;

    return filterEntry;
}
