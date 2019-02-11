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

#include "crumblepath.h"
#include "qtcassert.h"
#include "stylehelper.h"

#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QPixmapCache>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyleOptionButton>
#include <QStylePainter>

#include <algorithm>

namespace Utils {

class CrumblePathButton final : public QPushButton
{
    Q_OBJECT

public:
    enum SegmentType {
        FirstSegment,
        MiddleSegment,
        LastSegment,
        SingleSegment
    };

    explicit CrumblePathButton(const QString &title, QWidget *parent = nullptr);
    void setSegmentType(SegmentType type);
    void select(bool s);
    void setData(const QVariant &data);
    QVariant data() const;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    SegmentType m_segmentType = SingleSegment;
    QVariant m_data;
};

CrumblePathButton::CrumblePathButton(const QString &title, QWidget *parent)
    : QPushButton(title, parent)
{
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    setToolTip(title);
    setMinimumHeight(24);
    setMaximumHeight(24);
    setMouseTracking(true);
}

static QPixmap segmentPixmap(CrumblePathButton::SegmentType type, QStyle::State state)
{
    const QString segmentName = QLatin1String(
                type == CrumblePathButton::FirstSegment ?
                    "first" : type == CrumblePathButton::MiddleSegment ?
                        "middle" : type == CrumblePathButton::LastSegment ?
                            "last" : "single");

    const QIcon::Mode iconMode = state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled;
    const bool hover = state & QStyle::State_MouseOver || state & QStyle::State_HasFocus;

    const QString pixmapKey = QStringLiteral("crumblePath-segment-%1-iconMode-%2-hover-%3")
            .arg(segmentName).arg(iconMode).arg(hover);
    QPixmap pixmap;
    if (!QPixmapCache::find(pixmapKey, &pixmap)) {
        const QString maskFileName = QStringLiteral(":/utils/images/crumblepath-segment-%1%2.png")
                .arg(segmentName).arg(QLatin1String(hover ? "-hover" : ""));
        pixmap = Icon({{maskFileName, Theme::IconsBaseColor}}).pixmap(iconMode);
        QPixmapCache::insert(pixmapKey, pixmap);
    }

    return pixmap;
}

void CrumblePathButton::paintEvent(QPaintEvent*)
{
    QStyleOptionButton option;
    initStyleOption(&option);

    const QPixmap pixmap = segmentPixmap(m_segmentType, option.state);
    const int borderSize = 8;
    const int overlapSize = 2;
    const bool overlapLeft =
            m_segmentType == MiddleSegment || m_segmentType == LastSegment;
    const bool overlapRight =
            m_segmentType == FirstSegment || m_segmentType == MiddleSegment;
    QRect segmentRect = rect();
    segmentRect.setHeight(int(pixmap.height() / pixmap.devicePixelRatio()));
    segmentRect.moveCenter(rect().center());
    segmentRect.adjust(overlapLeft ? -overlapSize : 0, 0,
                     overlapRight ? overlapSize : 0, 0);

    QPainter p(this);
    StyleHelper::drawCornerImage(pixmap.toImage(), &p, segmentRect, borderSize, 0, borderSize, 0);

    const QPixmap middleSegmentPixmap = segmentPixmap(MiddleSegment, option.state);
    const int middleSegmentPixmapWidth =
            int(middleSegmentPixmap.width() / middleSegmentPixmap.devicePixelRatio());
    if (overlapLeft)
        p.drawPixmap(-middleSegmentPixmapWidth + overlapSize, segmentRect.top(), middleSegmentPixmap);
    if (overlapRight)
        p.drawPixmap(width() - overlapSize, segmentRect.top(), middleSegmentPixmap);

    if (option.state & QStyle::State_Enabled)
        option.palette.setColor(QPalette::ButtonText, creatorTheme()->color(Theme::PanelTextColorLight));
    else
        option.palette.setColor(QPalette::Disabled, QPalette::ButtonText, creatorTheme()->color(Theme::IconsDisabledColor));

    QStylePainter sp(this);
    if (option.state & QStyle::State_Sunken)
        sp.setOpacity(0.7);
    sp.drawControl(QStyle::CE_PushButtonLabel, option);
    if (option.features & QStyleOptionButton::HasMenu) {
        option.rect = segmentRect.adjusted(segmentRect.width() - 18, 3, -10, 0);
        StyleHelper::drawArrow(QStyle::PE_IndicatorArrowDown, &sp, &option);
    }
}

void CrumblePathButton::setSegmentType(SegmentType type)
{
    m_segmentType = type;
    update();
}

void CrumblePathButton::setData(const QVariant &data)
{
    m_data = data;
}

QVariant CrumblePathButton::data() const
{
    return m_data;
}

CrumblePath::CrumblePath(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(24);
    setMaximumHeight(24);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    auto layout = new QHBoxLayout(this);
    m_buttonsLayout = new QHBoxLayout;
    layout->addLayout(m_buttonsLayout);
    layout->addStretch(1);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    setStyleSheet("QPushButton { margin: 12; }");
}

CrumblePath::~CrumblePath()
{
    qDeleteAll(m_buttons);
    m_buttons.clear();
}

QVariant CrumblePath::dataForIndex(int index) const
{
    if (index > -1 && index < m_buttons.length())
        return m_buttons[index]->data();
    return QVariant();
}

QVariant CrumblePath::dataForLastIndex() const
{
    if (m_buttons.isEmpty())
        return QVariant();
    return m_buttons.last()->data();
}

int CrumblePath::length() const
{
    return m_buttons.length();
}

void CrumblePath::pushElement(const QString &title, const QVariant &data)
{
    auto *newButton = new CrumblePathButton(title, this);
    newButton->setData(data);
    m_buttonsLayout->addWidget(newButton);
    connect(newButton, &QAbstractButton::clicked, this, &CrumblePath::emitElementClicked);

    if (m_buttons.empty()) {
        newButton->setSegmentType(CrumblePathButton::SingleSegment);
    } else {
        m_buttons.last()->setSegmentType(m_buttons.count() > 1
                                         ? CrumblePathButton::MiddleSegment
                                         : CrumblePathButton::FirstSegment);
        newButton->setSegmentType(CrumblePathButton::LastSegment);
    }
    m_buttons.append(newButton);
}

void CrumblePath::addChild(const QString &title, const QVariant &data)
{
    QTC_ASSERT(!m_buttons.isEmpty(), return);

    auto *lastButton = m_buttons.last();

    QMenu *childList = lastButton->menu();
    if (!childList)
        childList = new QMenu(lastButton);

    auto *childAction = new QAction(title, lastButton);
    childAction->setData(data);
    connect(childAction, &QAction::triggered, this, &CrumblePath::emitElementClicked);
    childList->addAction(childAction);
    lastButton->setMenu(childList);
}

void CrumblePath::popElement()
{
    if (m_buttons.isEmpty())
        return;

    QWidget *last = m_buttons.last();
    m_buttons.removeLast();
    last->setParent(nullptr);
    last->deleteLater();

    if (!m_buttons.isEmpty())
        m_buttons.last()->setSegmentType(m_buttons.count() == 1
                                         ? CrumblePathButton::SingleSegment
                                         : CrumblePathButton::LastSegment);
}

void CrumblePath::clear()
{
    while (!m_buttons.isEmpty())
        popElement();
}

void CrumblePath::emitElementClicked()
{
    QObject *element = sender();
    if (auto *action = qobject_cast<QAction*>(element))
        emit elementClicked(action->data());
    else if (auto *button = qobject_cast<CrumblePathButton*>(element))
        emit elementClicked(button->data());
}

} // namespace Utils

#include "crumblepath.moc"
