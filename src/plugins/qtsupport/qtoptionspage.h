// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/treemodel.h>

#include <functional>
#include <variant>

namespace QtSupport {
class QtVersion;

namespace LinkWithQtSupport {
QTSUPPORT_EXPORT bool canLinkWithQt();
QTSUPPORT_EXPORT bool isLinkedWithQt();
QTSUPPORT_EXPORT Utils::FilePath linkedQt();
QTSUPPORT_EXPORT void linkWithQt();
}

namespace Internal {
class QtVersionItem : public Utils::TreeItem
{
public:
    explicit QtVersionItem(QtVersion *version) : m_version(version) {}
    explicit QtVersionItem(int versionId) : m_version(versionId) {}
    ~QtVersionItem();

    void setVersion(QtVersion *version);
    int uniqueId() const;
    QtVersion *version() const;
    void setChanged(bool changed);
    void setIsNameUnique(const std::function<bool(QtVersion *)> &isNameUnique);

    enum class Quality { Bad, Limited, Good }; // Keep sorted ascending by goodness.
    Quality quality() const;

private:
    QVariant data(int column, int role) const final;

    bool hasNonUniqueDisplayName() const { return m_isNameUnique && !m_isNameUnique(version()); }

    std::variant<QtVersion *, int> m_version;
    std::function<bool(QtVersion *)> m_isNameUnique;
    bool m_changed = false;
};

void setupQtSettingsPage();
}

} // QtSupport
