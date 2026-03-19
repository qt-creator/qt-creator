// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtsupport_global.h"

#include <utils/filepath.h>

#include <QVariant>

#include <functional>
#include <memory>

namespace QtSupport {
class QtVersion;

namespace LinkWithQtSupport {
QTSUPPORT_EXPORT bool canLinkWithQt();
QTSUPPORT_EXPORT bool isLinkedWithQt();
QTSUPPORT_EXPORT Utils::FilePath linkedQt();
QTSUPPORT_EXPORT void linkWithQt();
}

namespace Internal {

class QtVersionItem
{
public:
    QtVersionItem() = default;
    explicit QtVersionItem(QtVersion *version);

    void setIsNameUnique(const std::function<bool(QtVersion *)> &isNameUnique)
    {
        m_isNameUnique = isNameUnique;
    }

    int uniqueId() const;
    QtVersion *version() const { return m_version.get(); }

    QVariant data(int column, int role) const;

    friend bool operator==(const QtVersionItem &a, const QtVersionItem &b)
    {
        return a.m_version.get() == b.m_version.get()
            && a.m_changed == b.m_changed;
    }

    std::shared_ptr<QtVersion> m_version;
    std::function<bool(QtVersion *)> m_isNameUnique;
    bool m_changed = false;

private:
    bool hasNonUniqueDisplayName() const
    {
        return m_isNameUnique && !m_isNameUnique(version());
    }
};

QVariant qtVersionData(const QtVersion *version, int column, int role,
                       bool isChanged = false, bool hasNonUniqueName = false);

void setupQtSettingsPage();

} // Internal
} // QtSupport
