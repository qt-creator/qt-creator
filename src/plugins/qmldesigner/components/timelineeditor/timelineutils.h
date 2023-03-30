// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

inline bool isDeleteKey(int key)
{
    return (key == Qt::Key_Backspace) || (key == Qt::Key_Delete);
}

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

inline QIcon mergeIcons(const QIcon &on, const QIcon &off)
{
    QIcon out;
    out.addPixmap(on.pixmap({16, 16}), QIcon::Normal, QIcon::On);
    out.addPixmap(off.pixmap({16, 16}), QIcon::Normal, QIcon::Off);
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

} // namespace QmlDesigner
