/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

class QTCREATOR_UTILS_EXPORT MimeMagicRule
{
public:
    enum Type { Invalid = 0, String, RegExp, Host16, Host32, Big16, Big32, Little16, Little32, Byte };

    MimeMagicRule(Type type, const QByteArray &value, int startPos, int endPos,
                  const QByteArray &mask = QByteArray(), QString *errorString = 0);
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
    const QScopedPointer<MimeMagicRulePrivate> d;
};

} // Internal
} // Utils

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Utils::Internal::MimeMagicRule, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
