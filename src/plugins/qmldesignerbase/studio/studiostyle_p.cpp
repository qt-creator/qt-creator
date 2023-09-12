// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "studiostyle_p.h"

#include "studiostyle.h"

#include <utils/styleanimator.h>
#include <utils/theme/theme.h>

#include <QPainter>
#include <QStyleOption>
#include <QToolTip>

using namespace Utils;
using namespace QmlDesigner;

namespace {
inline QPixmap getPixmapFromIcon(
    const QIcon &icon, const QSize &size, bool enabled, bool active, bool checked)
{
    QIcon::Mode mode = enabled ? ((active) ? QIcon::Active : QIcon::Normal) : QIcon::Disabled;
    QIcon::State state = (checked) ? QIcon::On : QIcon::Off;
    return icon.pixmap(size, mode, state);
}

inline QColor studioTextColor(bool enabled, bool active, bool checked)
{
    Theme::Color themePenColorId = enabled ? (active ? (checked ? Theme::DSsubPanelBackground
                                                                : Theme::DSpanelBackground)
                                                     : Theme::DStextColor)
                                           : Theme::DStextColorDisabled;

    return creatorTheme()->color(themePenColorId);
}
}; // namespace

StudioStylePrivate::StudioStylePrivate(StudioStyle *q)
    : QObject(q)
    , q(q)
{
    auto color = [](Theme::Color c) { return creatorTheme()->color(c); };

    {
        stdPalette.setColorGroup(QPalette::Disabled,                        // group
                                 color(Theme::DStextColorDisabled),         // windowText
                                 color(Theme::DScontrolBackgroundDisabled), // button
                                 color(Theme::DScontrolOutlineDisabled),    // light
                                 color(Theme::DStextSelectedTextColor),     // dark
                                 color(Theme::DSstatusbarBackground),       // mid
                                 color(Theme::DStextColorDisabled),         // text
                                 color(Theme::DStextColorDisabled),         // brightText
                                 color(Theme::DStoolbarIcon_blocked),       // base
                                 color(Theme::DStoolbarIcon_blocked)        // window
        );

        stdPalette.setColorGroup(QPalette::Inactive,                  // group
                                 color(Theme::DStextColor),           // windowText
                                 color(Theme::DScontrolBackground),   // button
                                 color(Theme::DStoolbarBackground),   // light
                                 color(Theme::DSstatusbarBackground), // dark
                                 color(Theme::DScontrolBackground),   // mid
                                 color(Theme::DStextColor),           // text
                                 color(Theme::DStextColor),           // brightText
                                 color(Theme::DStoolbarBackground),   // base
                                 color(Theme::DStoolbarBackground)    // window
        );

        stdPalette.setColorGroup(QPalette::Active,                             // group
                                 color(Theme::DStextSelectedTextColor),        // windowText
                                 color(Theme::DSnavigatorItemBackgroundHover), // button
                                 color(Theme::DSstateBackgroundColor_hover),   // light
                                 color(Theme::DSpanelBackground),              // dark
                                 color(Theme::DSnavigatorItemBackgroundHover), // mid
                                 color(Theme::DStextSelectedTextColor),        // text
                                 color(Theme::DStextSelectedTextColor),        // brightText
                                 color(Theme::DStoolbarBackground),            // base
                                 color(Theme::DStoolbarBackground)             // window
        );

        stdPalette.setBrush(QPalette::ColorRole::ToolTipBase, color(Theme::DStoolbarBackground));
        stdPalette.setColor(QPalette::ColorRole::ToolTipText, color(Theme::DStextColor));
    }

    QToolTip::setPalette(stdPalette);
}

QList<const QObject *> StudioStylePrivate::animationTargets() const
{
    return m_animations.keys();
}

QStyleAnimation *StudioStylePrivate::animation(const QObject *target) const
{
    return m_animations.value(target, nullptr);
}

void StudioStylePrivate::startAnimation(QStyleAnimation *animation) const
{
    stopAnimation(animation->target());
    QObject::connect(animation,
                     &QObject::destroyed,
                     this,
                     &StudioStylePrivate::removeAnimation,
                     Qt::UniqueConnection);
    m_animations.insert(animation->target(), animation);
    animation->start();
}

void StudioStylePrivate::stopAnimation(const QObject *target) const
{
    QStyleAnimation *animation = m_animations.take(target);
    if (animation) {
        animation->stop();
        delete animation;
    }
}

void StudioStylePrivate::removeAnimation(const QObject *animationObject)
{
    if (animationObject)
        m_animations.remove(animationObject->parent());
}

StudioShortcut::StudioShortcut(const QStyleOptionMenuItem *option, const QString &shortcutText)
    : m_shortcutText(shortcutText)
    , m_enabled(option->state & QStyle::State_Enabled)
    , m_active(option->state & QStyle::State_Selected)
    , m_font(option->font)
    , m_fontMetrics(m_font)
    , m_defaultHeight(m_fontMetrics.height())
    , m_spaceConst(m_fontMetrics.boundingRect(".").width())
{
    reset();

    if (backspaceMatch(shortcutText).hasMatch() && option->styleObject)
        m_backspaceIcon = option->styleObject->property("backspaceIcon").value<QIcon>();
}

QSize StudioShortcut::getSize()
{
    if (m_isFirstParticle)
        calcResult();
    return m_size;
}

QPixmap StudioShortcut::getPixmap()
{
    if (!m_isFirstParticle && !m_pixmap.isNull())
        return m_pixmap;

    m_pixmap = QPixmap(getSize());
    m_pixmap.fill(Qt::transparent);
    QPainter painter(&m_pixmap);
    painter.setFont(m_font);
    QPen pPen = painter.pen();
    pPen.setColor(studioTextColor(m_enabled, m_active, false));
    painter.setPen(pPen);
    calcResult(&painter);
    painter.end();

    return m_pixmap;
}

void StudioShortcut::applySize(const QSize &itemSize)
{
    m_width += itemSize.width();
    m_height = std::max(m_height, itemSize.height());
    if (m_isFirstParticle)
        m_isFirstParticle = false;
    else
        m_width += m_spaceConst;
}

void StudioShortcut::addText(const QString &txt, QPainter *painter)
{
    if (txt.size()) {
        int textWidth = m_fontMetrics.horizontalAdvance(txt);
        QSize itemSize = {textWidth, m_defaultHeight};
        if (painter) {
            static const QTextOption textOption(Qt::AlignLeft | Qt::AlignVCenter);
            QRect placeRect({m_width, 0}, itemSize);
            painter->drawText(placeRect, txt, textOption);
        }
        applySize(itemSize);
    }
}

void StudioShortcut::addPixmap(const QPixmap &pixmap, QPainter *painter)
{
    if (painter)
        painter->drawPixmap(QRect({m_width, 0}, pixmap.size()), pixmap);

    applySize(pixmap.size());
}

void StudioShortcut::calcResult(QPainter *painter)
{
    reset();
#ifndef QT_NO_SHORTCUT
    if (!m_shortcutText.isEmpty()) {
        int fwdIndex = 0;

        QRegularExpressionMatch mMatch = backspaceMatch(m_shortcutText);
        int matchCount = mMatch.lastCapturedIndex();

        for (int i = 0; i <= matchCount; ++i) {
            QString mStr = mMatch.captured(i);
            QSize iconSize(m_defaultHeight * 3, m_defaultHeight);
            const QList<QSize> iconSizes = m_backspaceIcon.availableSizes();
            if (iconSizes.size())
                iconSize = iconSizes.last();
            double aspectRatio = (m_defaultHeight + .0) / iconSize.height();
            int newWidth = iconSize.width() * aspectRatio;

            QPixmap pixmap = getPixmapFromIcon(m_backspaceIcon,
                                               {newWidth, m_defaultHeight},
                                               m_enabled,
                                               m_active,
                                               false);

            int lIndex = m_shortcutText.indexOf(mStr, fwdIndex);
            int diffChars = lIndex - fwdIndex;
            addText(m_shortcutText.mid(fwdIndex, diffChars), painter);
            addPixmap(pixmap, painter);
            fwdIndex = lIndex + mStr.size();
        }
        addText(m_shortcutText.mid(fwdIndex), painter);
    }
#endif
    m_size = {m_width, m_height};
}

void StudioShortcut::reset()
{
    m_isFirstParticle = true;
    m_width = 0;
    m_height = 0;
}

QRegularExpressionMatch StudioShortcut::backspaceMatch(const QString &str) const
{
    static const QRegularExpression backspaceDetect("\\+*backspace\\+*",
                                                    QRegularExpression::CaseInsensitiveOption);
    return backspaceDetect.match(str);
}
