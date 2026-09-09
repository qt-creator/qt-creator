// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "chatfontscale.h"

#include "acpclientconstants.h"
#include "acpclienttr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <utils/fadingindicator.h>
#include <utils/markdownbrowser.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

#include <QAbstractScrollArea>
#include <QAction>
#include <QApplication>
#include <QChildEvent>
#include <QLabel>
#include <QLayout>
#include <QWheelEvent>

namespace AcpClient::Internal {

const char kFontZoomKey[] = "AcpClient/FontZoom";

constexpr int kDefaultZoom = 100;
constexpr qreal kMinScale = 0.5;
constexpr qreal kMaxScale = 4.0;

ChatFontScale::ChatFontScale()
    : m_scale(qBound(kMinScale,
                     Core::ICore::settings()->value(kFontZoomKey, kDefaultZoom).toInt() / 100.0,
                     kMaxScale))
{}

ChatFontScale &ChatFontScale::instance()
{
    static ChatFontScale theChatFontScale;
    return theChatFontScale;
}

qreal ChatFontScale::scale()
{
    return instance().m_scale;
}

void ChatFontScale::setScale(qreal scale)
{
    ChatFontScale &self = instance();
    const qreal bounded = qBound(kMinScale, scale, kMaxScale);
    if (qFuzzyCompare(self.m_scale, bounded))
        return;
    self.m_scale = bounded;
    Core::ICore::settings()->setValueWithDefault(kFontZoomKey, qRound(bounded * 100), kDefaultZoom);
    emit self.scaleChanged(bounded);
}

static void showZoomIndicator(QWidget *widget)
{
    if (!widget)
        return;
    auto *area = qobject_cast<QAbstractScrollArea *>(widget->parentWidget());
    if (area && area->viewport() == widget)
        widget = area;
    Utils::FadingIndicator::showText(
        widget,
        Tr::tr("Zoom: %1%").arg(qRound(ChatFontScale::scale() * 100)),
        Utils::FadingIndicator::SmallText);
}

static void zoomBy(qreal percent, QWidget *indicatorWidget)
{
    // Guarantee a step even for high resolution wheels.
    if (percent > 0 && percent < 1)
        percent = 1;
    else if (percent < 0 && percent > -1)
        percent = -1;

    const int current = qRound(ChatFontScale::scale() * 100);
    ChatFontScale::setScale((current + int(percent)) / 100.);
    showZoomIndicator(indicatorWidget);
}

void ChatFontScale::zoomIn()
{
    zoomBy(10, QApplication::focusWidget());
}

void ChatFontScale::zoomOut()
{
    zoomBy(-10, QApplication::focusWidget());
}

void ChatFontScale::resetZoom()
{
    setScale(1.0);
    showZoomIndicator(QApplication::focusWidget());
}

void setupChatZoomActions(QObject *guard)
{
    const Core::Context context(Constants::C_ACP_CHAT);

    const auto registerZoomAction = [&context, guard](const QString &text,
                                                      Utils::Id id,
                                                      void (*zoom)()) {
        auto *action = new QAction(text, guard);
        Core::ActionManager::registerAction(action, id, context);
        QObject::connect(action, &QAction::triggered, action, zoom);
    };

    registerZoomAction(Tr::tr("Increase Font Size"), Core::Constants::ZOOM_IN,
                       &ChatFontScale::zoomIn);
    registerZoomAction(Tr::tr("Decrease Font Size"), Core::Constants::ZOOM_OUT,
                       &ChatFontScale::zoomOut);
    registerZoomAction(Tr::tr("Reset Font Size"), Core::Constants::ZOOM_RESET,
                       &ChatFontScale::resetZoom);
}

class ChatZoomFilter : public QObject
{
public:
    static ChatZoomFilter *instance()
    {
        static ChatZoomFilter theChatZoomFilter;
        return &theChatZoomFilter;
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() != QEvent::Wheel)
            return QObject::eventFilter(watched, event);

        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        if (!(wheelEvent->modifiers() & Qt::ControlModifier))
            return QObject::eventFilter(watched, event);

        const qreal deltaY = wheelEvent->angleDelta().y() / 120.;
        if (qFuzzyIsNull(deltaY))
            return true;

        zoomBy(10. * deltaY, qobject_cast<QWidget *>(watched));
        return true;
    }
};

static void applyScaledFont(QWidget *widget, const QFont &baseFont)
{
    QFont font = baseFont;
    font.setPointSizeF(baseFont.pointSizeF() * ChatFontScale::scale());
    widget->setFont(font);
}

void setChatFont(QWidget *widget, const QFont &baseFont)
{
    QTC_ASSERT(widget, return);
    applyScaledFont(widget, baseFont);
    QObject::connect(&ChatFontScale::instance(), &ChatFontScale::scaleChanged, widget,
                     [widget, baseFont] { applyScaledFont(widget, baseFont); });
}

void setChatFont(QWidget *widget, qreal factor)
{
    QTC_ASSERT(widget, return);
    QFont baseFont = QApplication::font();
    baseFont.setPointSizeF(baseFont.pointSizeF() * factor);
    setChatFont(widget, baseFont);
}

void setChatTextFormat(QLabel *label, const Utils::StyleHelper::TextFormat &format)
{
    QTC_ASSERT(label, return);
    Utils::StyleHelper::applyTf(label, format);
    // applyTf pins the label to an unscaled line height, which would cut the
    // scaled text off.
    const int lineHeight = format.lineHeight();
    const auto applyLineHeight = [label, lineHeight] {
        label->setFixedHeight(qRound(lineHeight * ChatFontScale::scale()));
    };
    applyLineHeight();
    QObject::connect(&ChatFontScale::instance(), &ChatFontScale::scaleChanged, label,
                     applyLineHeight);
    setChatFont(label, format.font());
}

qreal chatRadius(qreal radius)
{
    return radius * ChatFontScale::scale();
}

const char kBaseSpacingProperty[] = "acpBaseSpacing";
const char kBaseMarginsProperty[] = "acpBaseMargins";
const char kAppliedScaleProperty[] = "acpAppliedScale";

// A layout without an own spacing keeps following the style.
static int scaledSpacing(int base, qreal scale)
{
    return base < 0 ? base : qRound(base * scale);
}

static QMargins scaledMargins(const QMargins &base, qreal scale)
{
    return QMargins(qRound(base.left() * scale), qRound(base.top() * scale),
                    qRound(base.right() * scale), qRound(base.bottom() * scale));
}

static void applyScaledSpacing(QLayout *layout)
{
    if (!layout)
        return;

    const qreal scale = ChatFontScale::scale();

    // A layout is reached as soon as it is attached to its widget, which is
    // before the widget's constructor configures it, so the base is whatever
    // the style hands out at that point. What was written here last is the
    // base taken at the scale it was written at; anything else comes from the
    // widget itself and is the new base.
    const QVariant baseSpacing = layout->property(kBaseSpacingProperty);
    const QMargins baseMargins = layout->property(kBaseMarginsProperty).value<QMargins>();
    const qreal appliedScale = layout->property(kAppliedScaleProperty).toReal();
    if (!baseSpacing.isValid()
        || layout->spacing() != scaledSpacing(baseSpacing.toInt(), appliedScale)
        || layout->contentsMargins() != scaledMargins(baseMargins, appliedScale)) {
        layout->setProperty(kBaseSpacingProperty, layout->spacing());
        layout->setProperty(kBaseMarginsProperty, QVariant::fromValue(layout->contentsMargins()));
    }

    const int spacing = scaledSpacing(layout->property(kBaseSpacingProperty).toInt(), scale);
    const QMargins margins = scaledMargins(
        layout->property(kBaseMarginsProperty).value<QMargins>(), scale);

    if (layout->spacing() != spacing)
        layout->setSpacing(spacing);
    if (layout->contentsMargins() != margins)
        layout->setContentsMargins(margins);
    layout->setProperty(kAppliedScaleProperty, scale);

    for (int i = 0; i < layout->count(); ++i)
        applyScaledSpacing(layout->itemAt(i)->layout());
}

// Spacings and margins are set from the spacing tokens when a widget is built,
// so they do not follow the chat scale by themselves. This keeps them in
// proportion with the text, for the widgets present now and for the ones the
// conversation grows later.
class ChatSpacingScaler : public QObject
{
public:
    explicit ChatSpacingScaler(QWidget *root)
        : QObject(root)
    {
        watch(root);
        connect(&ChatFontScale::instance(), &ChatFontScale::scaleChanged, this, [this, root] {
            rescale(root);
        });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::ChildAdded:
            // A widget arrives before it has a layout, and its layout gets its
            // items only afterwards, so both sides are scaled again here.
            if (auto *widget = qobject_cast<QWidget *>(watched))
                applyScaledSpacing(widget->layout());
            if (auto *child = qobject_cast<QWidget *>(static_cast<QChildEvent *>(event)->child()))
                watch(child);
            break;
        case QEvent::LayoutRequest:
            if (auto *widget = qobject_cast<QWidget *>(watched))
                applyScaledSpacing(widget->layout());
            break;
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void watch(QWidget *widget)
    {
        widget->installEventFilter(this);
        applyScaledSpacing(widget->layout());
        const QList<QWidget *> children = widget->findChildren<QWidget *>();
        for (QWidget *child : children) {
            child->installEventFilter(this);
            applyScaledSpacing(child->layout());
        }
    }

    void rescale(QWidget *widget)
    {
        applyScaledSpacing(widget->layout());
        const QList<QWidget *> children = widget->findChildren<QWidget *>();
        for (QWidget *child : children)
            applyScaledSpacing(child->layout());
    }
};

void setChatSpacing(QWidget *widget)
{
    QTC_ASSERT(widget, return);
    new ChatSpacingScaler(widget);
}

void setupChatBrowser(Utils::MarkdownBrowser *browser)
{
    QTC_ASSERT(browser, return);
    browser->setScale(ChatFontScale::scale());
    QObject::connect(&ChatFontScale::instance(), &ChatFontScale::scaleChanged, browser,
                     [browser](qreal scale) { browser->setScale(scale); });
    enableChatZoom(browser);
}

void enableChatZoom(QWidget *widget)
{
    QTC_ASSERT(widget, return);
    widget->installEventFilter(ChatZoomFilter::instance());
    if (auto *area = qobject_cast<QAbstractScrollArea *>(widget))
        area->viewport()->installEventFilter(ChatZoomFilter::instance());
}

} // namespace AcpClient::Internal
