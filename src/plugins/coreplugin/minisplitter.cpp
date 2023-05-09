// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "minisplitter.h"

#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QPaintEvent>
#include <QPainter>
#include <QSplitterHandle>

namespace Core {
namespace Internal {

class MiniSplitterHandle : public QSplitterHandle
{
public:
    MiniSplitterHandle(Qt::Orientation orientation, QSplitter *parent, bool lightColored = false)
            : QSplitterHandle(orientation, parent),
              m_lightColored(lightColored)
    {
        setMask(QRegion(contentsRect()));
        setAttribute(Qt::WA_MouseNoMask, true);
    }
protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_lightColored;
};

} // namespace Internal
} // namespace Core

using namespace Core;
using namespace Core::Internal;

void MiniSplitterHandle::resizeEvent(QResizeEvent *event)
{
    if (orientation() == Qt::Horizontal)
        setContentsMargins(2, 0, 2, 0);
    else
        setContentsMargins(0, 2, 0, 2);
    setMask(QRegion(contentsRect()));
    QSplitterHandle::resizeEvent(event);
}

void MiniSplitterHandle::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const QColor color = Utils::creatorTheme()->color(
                m_lightColored ? Utils::Theme::FancyToolBarSeparatorColor
                               : Utils::Theme::SplitterColor);
    painter.fillRect(event->rect(), color);
}

/*!
    \class Core::MiniSplitter
    \inheaderfile coreplugin/minisplitter.h
    \inmodule QtCreator

    \brief The MiniSplitter class is a simple helper-class to obtain
    \macos style 1-pixel wide splitters.
*/

/*!
    \enum Core::MiniSplitter::SplitterStyle
    This enum value holds the splitter style.

    \value Dark  Dark style.
    \value Light Light style.
*/

QSplitterHandle *MiniSplitter::createHandle()
{
    return new MiniSplitterHandle(orientation(), this, m_style == Light);
}

MiniSplitter::MiniSplitter(QWidget *parent, SplitterStyle style)
    : QSplitter(parent),
      m_style(style)
{
    setHandleWidth(1);
    setChildrenCollapsible(false);
    setProperty(Utils::StyleHelper::C_MINI_SPLITTER, true);
}

MiniSplitter::MiniSplitter(Qt::Orientation orientation, QWidget *parent, SplitterStyle style)
    : QSplitter(orientation, parent),
      m_style(style)
{
    setHandleWidth(1);
    setChildrenCollapsible(false);
    setProperty(Utils::StyleHelper::C_MINI_SPLITTER, true);
}

/*!
    \class Core::NonResizingSplitter
    \inheaderfile coreplugin/minisplitter.h
    \inmodule QtCreator

    \brief The NonResizingSplitter class is a MiniSplitter that keeps its
    first widget's size fixed when it is resized.
*/

/*!
    Constructs a non-resizing splitter with \a parent and \a style.

    The default style is MiniSplitter::Light.
*/
NonResizingSplitter::NonResizingSplitter(QWidget *parent, SplitterStyle style)
    : MiniSplitter(parent, style)
{
}

/*!
    \internal
*/
void NonResizingSplitter::resizeEvent(QResizeEvent *ev)
{
    // bypass QSplitter magic
    int leftSplitWidth = qMin(sizes().at(0), ev->size().width());
    int rightSplitWidth = qMax(0, ev->size().width() - leftSplitWidth);
    setSizes(QList<int>() << leftSplitWidth << rightSplitWidth);
    QWidget::resizeEvent(ev);
}
