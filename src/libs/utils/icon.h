// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "theme/theme.h"

#include <QIcon>
#include <QList>
#include <QPair>

QT_BEGIN_NAMESPACE
class QColor;
class QPixmap;
class QString;
QT_END_NAMESPACE

namespace Utils {

using IconMaskAndColor = QPair<FilePath, Theme::Color>;

// Returns a recolored icon with shadow and custom disabled state for a
// series of grayscalemask|Theme::Color mask pairs
class QTCREATOR_UTILS_EXPORT Icon
{
public:
    enum IconStyleOption {
        None = 0,
        Tint = 1,
        DropShadow = 2,
        PunchEdges = 4,

        ToolBarStyle = Tint | DropShadow | PunchEdges,
        MenuTintedStyle = Tint | PunchEdges
    };

    Q_DECLARE_FLAGS(IconStyleOptions, IconStyleOption)

    Icon();
    Icon(const QList<IconMaskAndColor> &args, IconStyleOptions style = ToolBarStyle);
    Icon(const FilePath &imageFileName);

    QIcon icon() const;
    // Same as icon() but without disabled state.
    QPixmap pixmap(QIcon::Mode iconMode = QIcon::Normal) const;

    // Try to avoid it. it is just there for special API cases in Qt Creator
    // where icons are still defined as filename.
    FilePath imageFilePath() const;

    // Returns either the classic or a themed icon depending on
    // the current Theme::FlatModeIcons flag.
    static QIcon sideBarIcon(const Icon &classic, const Icon &flat);
    // Like sideBarIcon plus added action mode for the flat icon
    static QIcon modeIcon(const Icon &classic, const Icon &flat, const Icon &flatActive);

    // Combined icon pixmaps in Normal and Disabled states from several Icons
    static QIcon combinedIcon(const QList<QIcon> &icons);
    static QIcon combinedIcon(const QList<Icon> &icons);

    static QIcon fromTheme(const QString &name);

private:
    QList<IconMaskAndColor> m_iconSourceList;
    IconStyleOptions m_style = None;
    mutable int m_lastDevicePixelRatio = -1;
    mutable QIcon m_lastIcon;
};

} // namespace Utils

Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::Icon::IconStyleOptions)
