/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "panelswidget.h"

#include "propertiespanel.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/qtcassert.h>

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

    if (creatorTheme()->widgetStyle() == Theme::StyleDefault) {
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
    QScrollArea(parent),
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

    // The layout holding the individual panels:
    QVBoxLayout *topLayout = new QVBoxLayout(m_root);
    topLayout->setMargin(0);
    topLayout->setSpacing(0);

    m_layout = new QGridLayout;
    m_layout->setColumnMinimumWidth(0, ICON_SIZE + 4);
    m_layout->setSpacing(0);
    topLayout->addLayout(m_layout);
    topLayout->addStretch(100);

    setWidget(m_root);
    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    setFocusPolicy(Qt::NoFocus);
}

PanelsWidget::~PanelsWidget()
{
    qDeleteAll(m_panels);
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
void PanelsWidget::addPropertiesPanel(PropertiesPanel *panel)
{
    QTC_ASSERT(panel, return);

    const int headerRow = m_layout->rowCount();

    // icon:
    if (!panel->icon().isNull()) {
        QLabel *iconLabel = new QLabel(m_root);
        iconLabel->setPixmap(panel->icon().pixmap(ICON_SIZE, ICON_SIZE));
        iconLabel->setContentsMargins(0, ABOVE_HEADING_MARGIN, 0, 0);
        m_layout->addWidget(iconLabel, headerRow, 0, 3, 1, Qt::AlignTop | Qt::AlignHCenter);
    }

    // name:
    QLabel *nameLabel = new QLabel(m_root);
    nameLabel->setText(panel->displayName());
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
    const int lineRow(headerRow + 1);
    QWidget *line = new OnePixelBlackLine(m_root);
    m_layout->addWidget(line, lineRow, 1, 1, -1, Qt::AlignTop);

    // add the widget:
    const int widgetRow(lineRow + 1);
    addPanelWidget(panel, widgetRow);
}

void PanelsWidget::addPanelWidget(PropertiesPanel *panel, int row)
{
    QWidget *widget = panel->widget();
    widget->setContentsMargins(PANEL_LEFT_MARGIN,
                               ABOVE_CONTENTS_MARGIN, 0,
                               BELOW_CONTENTS_MARGIN);
    widget->setParent(m_root);
    m_layout->addWidget(widget, row, 0, 1, 2);

    m_panels.append(panel);
}
