// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "usercategory.h"

#include <QJsonObject>

namespace QmlDesigner {

class ContentLibraryItem;

class UserItemCategory : public UserCategory
{
    Q_OBJECT

public:
    UserItemCategory(const QString &title, const Utils::FilePath &bundlePath, const QString &bundleId);

    void loadBundle(bool force) override;
    void filter(const QString &searchText) override;

    QStringList sharedFiles() const;

    QJsonObject &bundleObjRef();

private:
    QString m_bundleId;
    QJsonObject m_bundleObj;
    QStringList m_sharedFiles;

};

} // namespace QmlDesigner
