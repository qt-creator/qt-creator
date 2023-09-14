// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QObject>
#include <QPalette>
#include <QPixmap>
#include <QProxyStyle>
#include <QRegularExpressionMatch>
#include <QSize>

class QStyleOptionMenuItem;
class QPainter;

namespace Utils {
class StyleAnimation;
class QStyleAnimation;
} // namespace Utils

namespace QmlDesigner {
class StudioStyle;

class StudioStylePrivate : public QObject
{
    Q_OBJECT
    using StyleAnimation = Utils::StyleAnimation;
    using QStyleAnimation = Utils::QStyleAnimation;

public:
    explicit StudioStylePrivate(StudioStyle *q);

    QList<const QObject *> animationTargets() const;
    QStyleAnimation *animation(const QObject *target) const;
    void startAnimation(QStyleAnimation *animation) const;
    void stopAnimation(const QObject *target) const;

    QPalette stdPalette;

private:
    StudioStyle *q = nullptr;
    mutable QHash<const QObject *, QStyleAnimation *> m_animations;

private slots:
    void removeAnimation(const QObject *animationObject);
};

struct StudioShortcut
{
    StudioShortcut(const QStyleOptionMenuItem *option, const QString &shortcutText);
    QSize getSize();
    QPixmap getPixmap();

private:
    void applySize(const QSize &itemSize);
    void addText(const QString &txt, QPainter *painter = nullptr);
    void addPixmap(const QPixmap &pixmap, QPainter *painter = nullptr);
    void calcResult(QPainter *painter = nullptr);
    void reset();
    QRegularExpressionMatch backspaceMatch(const QString &str) const;

    const QString m_shortcutText;
    const bool m_enabled;
    const bool m_active;
    const QFont m_font;
    const QFontMetrics m_fontMetrics;
    const int m_defaultHeight;
    const int m_spaceConst;

    QIcon m_backspaceIcon;
    bool m_isFirstParticle = true;

    int m_width = 0;
    int m_height = 0;
    QSize m_size;
    QPixmap m_pixmap;
};

} // namespace QmlDesigner
