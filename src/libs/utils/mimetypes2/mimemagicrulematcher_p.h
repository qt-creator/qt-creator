// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "mimemagicrule_p.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

namespace Utils {

class MimeMagicRuleMatcher
{
public:
    explicit MimeMagicRuleMatcher(const QString &mime, unsigned priority = 65535);

    void swap(MimeMagicRuleMatcher &other) noexcept
    {
        qSwap(m_list,     other.m_list);
        qSwap(m_priority, other.m_priority);
        qSwap(m_mimetype, other.m_mimetype);
    }

    bool operator==(const MimeMagicRuleMatcher &other) const;

    void addRule(const MimeMagicRule &rule);
    void addRules(const QList<MimeMagicRule> &rules);
    QList<MimeMagicRule> magicRules() const;

    bool matches(const QByteArray &data) const;

    unsigned priority() const;

    QString mimetype() const { return m_mimetype; }

private:
    QList<MimeMagicRule> m_list;
    unsigned m_priority;
    QString m_mimetype;
};

} // namespace Utils

QT_BEGIN_NAMESPACE
Q_DECLARE_SHARED(Utils::MimeMagicRuleMatcher)
QT_END_NAMESPACE
