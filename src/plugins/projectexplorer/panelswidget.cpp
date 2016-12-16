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

#include "panelswidget.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

namespace {
const int ICON_SIZE(64);

const int ABOVE_HEADING_MARGIN(10);
const int ABOVE_CONTENTS_MARGIN(4);
const int BELOW_CONTENTS_MARGIN(16);
const int PANEL_LEFT_MARGIN = 70;

} // anonymous namespace
///
// OnePixelBlackLine
///
/// \brief The OnePixelBlackLine class

using namespace ProjectExplorer;
using namespace Utils;

namespace {
class OnePixelBlackLine : public QWidget
{
public:
    OnePixelBlackLine(QWidget *parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(1);
        setMaximumHeight(1);
    }
    void paintEvent(QPaintEvent *e)
    {
        Q_UNUSED(e);
        QPainter p(this);
        QColor fillColor = creatorTheme()->color(Theme::PanelsWidgetSeparatorLineColor);
        p.fillRect(contentsRect(), fillColor);
    }
};

class RootWidget : public QWidget
{
public:
    RootWidget(QWidget *parent) : QWidget(parent) {
        setFocusPolicy(Qt::NoFocus);
    }
    void paintEvent(QPaintEvent *);
};

void RootWidget::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    if (!creatorTheme()->flag(Theme::FlatToolBars)) {
        // draw separator line to the right of the settings panel
        QPainter painter(this);
        QColor light = StyleHelper::mergedColors(
                    palette().button().color(), Qt::white, 30);
        QColor dark = StyleHelper::mergedColors(
                    palette().button().color(), Qt::black, 85);

        painter.setPen(light);
        painter.drawLine(rect().topRight(), rect().bottomRight());
        painter.setPen(dark);
        painter.drawLine(rect().topRight() - QPoint(1,0), rect().bottomRight() - QPoint(1,0));
    }
}
}

///
// PanelsWidget
///

PanelsWidget::PanelsWidget(QWidget *parent) :
    QWidget(parent),
    m_root(new RootWidget(this))
{
    // We want a 900px wide widget with and the scrollbar at the
    // side of the screen.
    m_root->setMaximumWidth(900);
    m_root->setContentsMargins(0, 0, 40, 0);
    QPalette pal;
    QColor background = StyleHelper::mergedColors(
                palette().window().color(), Qt::white, 85);
    pal.setColor(QPalette::All, QPalette::Window, background.darker(102));
    setPalette(pal);
    pal.setColor(QPalette::All, QPalette::Window, background);
    m_root->setPalette(pal);

    m_scroller = new QScrollArea(this);
    m_scroller->setWidget(m_root);
    m_scroller->setFrameStyle(QFrame::NoFrame);
    m_scroller->setWidgetResizable(true);
    m_scroller->setFocusPolicy(Qt::NoFocus);

    // The layout holding the individual panels:
    auto topLayout = new QVBoxLayout(m_root);
    topLayout->setMargin(0);
    topLayout->setSpacing(0);

    m_layout = new QGridLayout;
    m_layout->setColumnMinimumWidth(0, ICON_SIZE + 4);
    m_layout->setSpacing(0);

    topLayout->addLayout(m_layout);
    topLayout->addStretch(100);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(new Utils::StyledBar(this));
    layout->addWidget(m_scroller);

    //layout->addWidget(new FindToolBarPlaceHolder(this));
}

PanelsWidget::PanelsWidget(const QString &displayName, const QIcon &icon, QWidget *widget)
    : PanelsWidget(nullptr)
{
    addPropertiesPanel(displayName, icon, widget);
}

PanelsWidget::~PanelsWidget()
{
}

/*
 * Add a widget with heading information into the grid
 * layout of the PanelsWidget.
 *
 *     ...
 * +--------+-------------------------------------------+ ABOVE_HEADING_MARGIN
 * | icon   | name                                      |
 * +        +-------------------------------------------+
 * |        | line                                      |
 * +        +-------------------------------------------+ ABOVE_CONTENTS_MARGIN
 * |        | widget (with contentsmargins adjusted!)   |
 * +--------+-------------------------------------------+ BELOW_CONTENTS_MARGIN
 */
void PanelsWidget::addPropertiesPanel(const QString &displayName, const QIcon &icon, QWidget *widget)
{
    const int headerRow = m_layout->rowCount();

    // icon:
    if (!icon.isNull()) {
        auto iconLabel = new QLabel(m_root);
        iconLabel->setPixmap(icon.pixmap(ICON_SIZE, ICON_SIZE));
        iconLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
        m_layout->addWidget(iconLabel, headerRow, 0, 3, 1, Qt::AlignTop | Qt::AlignHCenter);
    }

    // name:
    auto nameLabel = new QLabel(m_root);
    nameLabel->setText(displayName);
    QPalette palette = nameLabel->palette();
    for (int i = QPalette::Active; i < QPalette::NColorGroups; ++i ) {
        // FIXME: theming
        QColor foregroundColor = palette.color(QPalette::ColorGroup(i), QPalette::Foreground);
        foregroundColor.setAlpha(110);
        palette.setBrush(QPalette::ColorGroup(i), QPalette::Foreground, foregroundColor);
    }
    nameLabel->setPalette(palette);
    nameLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
    QFont f = nameLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.6);
    nameLabel->setFont(f);
    m_layout->addWidget(nameLabel, headerRow, 1, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);

    // line:
    const int lineRow = headerRow + 1;
    auto line = new OnePixelBlackLine(m_root);
    m_layout->addWidget(line, lineRow, 1, 1, -1, Qt::AlignTop);

    // add the widget:
    const int widgetRow = lineRow + 1;

    widget->setContentsMargins(PANEL_LEFT_MARGIN,
                               ABOVE_CONTENTS_MARGIN, 0,
                               BELOW_CONTENTS_MARGIN);
    widget->setParent(m_root);
    m_layout->addWidget(widget, widgetRow, 0, 1, 2);
}
