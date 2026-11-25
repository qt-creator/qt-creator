// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contextitem.h"

#include "acpclienttr.h"

#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAbstractButton>
#include <QBuffer>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>

using namespace Utils;
using namespace Utils::StyleHelper::SpacingTokens;

namespace AcpClient::Internal {

/*!
    \class AcpClient::Internal::ContextItem
    \internal

    A chip that names a piece of context, with an \a icon, and a button that
    removes it unless setRemovable() turned that off.
*/

ContextItem::ContextItem(const QString &text, const QIcon &icon, QWidget *parent,
                         bool withBorder, int iconSize)
    : QWidget(parent)
    , m_withBorder(withBorder)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(PaddingHM, PaddingVXs, PaddingHXs, PaddingVXs);
    layout->setSpacing(PaddingHXs);

    if (!icon.isNull()) {
        auto iconLabel = new QLabel;
        // Render at the device pixel ratio so the preview stays crisp on
        // high-DPI displays.
        iconLabel->setPixmap(icon.pixmap(QSize(iconSize, iconSize), devicePixelRatioF()));
        layout->addWidget(iconLabel);
    }

    auto label = new QLabel(text);
    layout->addWidget(label);

    auto closeBtn = new QtcIconButton();
    closeBtn->setMaximumSize(20, 20);
    closeBtn->setIcon(Icons::CLOSE_TOOLBAR.icon());
    closeBtn->setContentsMargins(0, 0, 0, 0);
    closeBtn->setToolTip(Tr::tr("Remove Context"));
    connect(closeBtn, &QAbstractButton::clicked, this, &ContextItem::removeRequested);
    layout->addWidget(closeBtn);
    m_closeButton = closeBtn;
    setCursor(Qt::PointingHandCursor);
}

void ContextItem::setRemovable(bool removable)
{
    m_closeButton->setVisible(removable);
    // Without the button, the padding on the right matches the one on the left.
    layout()->setContentsMargins(PaddingHM, PaddingVXs, removable ? PaddingHXs : PaddingHM,
                                 PaddingVXs);
}

// Enables a rich tooltip that shows the given image at (up to) its original
// resolution. The tooltip HTML is built lazily on first hover.
void ContextItem::setPreviewImage(const QImage &image)
{
    m_previewImage = image;
}

bool ContextItem::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip && !m_previewImage.isNull() && !m_previewBuilt) {
        buildPreviewTooltip();
        m_previewBuilt = true;
    }
    return QWidget::event(e);
}

void ContextItem::mousePressEvent(QMouseEvent *event)
{
    // Emit queued: a handler may rebuild the context bar and delete this
    // widget, so the click must not run while we are still inside the
    // event (which would return into a deleted 'this').
    if (event->button() == Qt::LeftButton)
        QMetaObject::invokeMethod(this, &ContextItem::clicked, Qt::QueuedConnection);
    QWidget::mousePressEvent(event);
}

void ContextItem::paintEvent(QPaintEvent *)
{
    if (!m_withBorder)
        return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    StyleHelper::drawCardBg(
        &p, rect(), QBrush(), creatorColor(Theme::Token_Foreground_Muted), RadiusS);
}

void ContextItem::buildPreviewTooltip()
{
    // Show the image at its original resolution, but never larger than the
    // available screen area so the tooltip stays usable.
    QSize shown = m_previewImage.size();
    if (QScreen *s = screen()) {
        const QSize avail = s->availableSize() * 0.9;
        if (shown.width() > avail.width() || shown.height() > avail.height())
            shown.scale(avail, Qt::KeepAspectRatio);
    }

    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    m_previewImage.save(&buffer, "PNG");

    const QString html
        = QStringLiteral("<p>%1 x %2</p><img src=\"data:image/png;base64,%3\" "
                         "width=\"%4\" height=\"%5\">")
              .arg(m_previewImage.width())
              .arg(m_previewImage.height())
              .arg(QString::fromLatin1(png.toBase64()))
              .arg(shown.width())
              .arg(shown.height());
    setToolTip(html);
}

} // namespace AcpClient::Internal
