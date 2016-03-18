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

#pragma once

#include "texteditor_global.h"

#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

/**
 * Settings that describe how the text editor behaves. This does not include
 * the TabSettings and StorageSettings.
 */
class TEXTEDITOR_EXPORT BehaviorSettings
{
public:
    BehaviorSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    void toMap(const QString &prefix, QVariantMap *map) const;
    void fromMap(const QString &prefix, const QVariantMap &map);

    bool equals(const BehaviorSettings &bs) const;

    bool m_mouseHiding;
    bool m_mouseNavigation;
    bool m_scrollWheelZooming;
    bool m_constrainHoverTooltips;
    bool m_camelCaseNavigation;
    bool m_keyboardTooltips;
    bool m_smartSelectionChanging;
};

inline bool operator==(const BehaviorSettings &t1, const BehaviorSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const BehaviorSettings &t1, const BehaviorSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor
