// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR LGPL-3.0

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

#include <utils/utils_global.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>

namespace Utils {

class MimeType;

namespace Internal {
class MimeMagicRulePrivate;
}

class QTCREATOR_UTILS_EXPORT MimeMagicRule
{
public:
    enum Type { Invalid = 0, String, RegExp, Host16, Host32, Big16, Big32, Little16, Little32, Byte };

    MimeMagicRule(Type type, const QByteArray &value, int startPos, int endPos,
                  const QByteArray &mask = QByteArray(), QString *errorString = nullptr);
    MimeMagicRule(const MimeMagicRule &other);
    ~MimeMagicRule();

    MimeMagicRule &operator=(const MimeMagicRule &other);

    bool operator==(const MimeMagicRule &other) const;

    Type type() const;
    QByteArray value() const;
    int startPos() const;
    int endPos() const;
    QByteArray mask() const;

    bool isValid() const;

    bool matches(const QByteArray &data) const;

    QList<MimeMagicRule> m_subMatches;

    static Type type(const QByteArray &type);
    static QByteArray typeName(Type type);

    static bool matchSubstring(const char *dataPtr, int dataSize, int rangeStart, int rangeLength, int valueLength, const char *valueData, const char *mask);

private:
    const QScopedPointer<Internal::MimeMagicRulePrivate> d;
};

} // Utils

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Utils::MimeMagicRule, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
