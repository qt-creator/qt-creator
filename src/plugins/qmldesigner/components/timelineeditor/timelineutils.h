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

#include <istream>
#include <utils/icon.h>
#include <vector>
#include <QDataStream>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QEvent)

namespace QmlDesigner {

namespace TimelineUtils {

enum class Side { Top, Right, Bottom, Left };

template<typename T>
inline T clamp(const T &value, const T &lo, const T &hi)
{
    return value < lo ? lo : value > hi ? hi : value;
}

template<typename T>
inline T lerp(const T &blend, const T &lhs, const T &rhs)
{
    static_assert(std::is_floating_point<T>::value,
                  "TimelineUtils::lerp: For floating point types only!");
    return blend * lhs + (1.0 - blend) * rhs;
}

template<typename T>
inline T reverseLerp(const T &val, const T &lhs, const T &rhs)
{
    static_assert(std::is_floating_point<T>::value,
                  "TimelineUtils::reverseLerp: For floating point types only!");
    return (val - rhs) / (lhs - rhs);
}

inline QIcon mergeIcons(const Utils::Icon &on, const Utils::Icon &off)
{
    QIcon out;
    out.addPixmap(on.pixmap(), QIcon::Normal, QIcon::On);
    out.addPixmap(off.pixmap(), QIcon::Normal, QIcon::Off);
    return out;
}

class DisableContextMenu : public QObject
{
    Q_OBJECT

public:
    explicit DisableContextMenu(QObject *parent = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;
};

} // End namespace TimelineUtils.

template<typename T>
inline std::istream &operator>>(std::istream &stream, std::vector<T> &vec)
{
    quint64 s;
    stream >> s;

    vec.clear();
    vec.reserve(s);

    T val;
    for (quint64 i = 0; i < s; ++i) {
        stream >> val;
        vec.push_back(val);
    }
    return stream;
}

template<typename T>
inline QDataStream &operator<<(QDataStream &stream, const std::vector<T> &vec)
{
    stream << static_cast<quint64>(vec.size());
    for (const auto &elem : vec)
        stream << elem;

    return stream;
}

template<typename T>
inline QDataStream &operator>>(QDataStream &stream, std::vector<T> &vec)
{
    quint64 s;
    stream >> s;

    vec.clear();
    vec.reserve(s);

    T val;
    for (quint64 i = 0; i < s; ++i) {
        stream >> val;
        vec.push_back(val);
    }
    return stream;
}

} // namespace QmlDesigner
