/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of QLiteHtml.
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

#include "qlitehtmlwidget.h"

#include "container_qpainter.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QTimer>

#include <litehtml.h>

const int kScrollBarStep = 40;

// TODO copied from litehtml/include/master.css
const char mastercss[] = R"RAW(
html {
    display: block;
height:100%;
width:100%;
position: relative;
}

head {
    display: none
}

meta {
    display: none
}

title {
    display: none
}

link {
    display: none
}

style {
    display: none
}

script {
    display: none
}

body {
display:block;
    margin:8px;
    height:100%;
width:100%;
}

p {
display:block;
    margin-top:1em;
    margin-bottom:1em;
}

b, strong {
display:inline;
    font-weight:bold;
}

i, em {
display:inline;
    font-style:italic;
}

center
{
    text-align:center;
display:block;
}

a:link
{
    text-decoration: underline;
color: #00f;
cursor: pointer;
}

h1, h2, h3, h4, h5, h6, div {
display:block;
}

h1 {
    font-weight:bold;
    margin-top:0.67em;
    margin-bottom:0.67em;
    font-size: 2em;
}

h2 {
    font-weight:bold;
    margin-top:0.83em;
    margin-bottom:0.83em;
    font-size: 1.5em;
}

h3 {
    font-weight:bold;
    margin-top:1em;
    margin-bottom:1em;
    font-size:1.17em;
}

h4 {
    font-weight:bold;
    margin-top:1.33em;
    margin-bottom:1.33em
}

h5 {
    font-weight:bold;
    margin-top:1.67em;
    margin-bottom:1.67em;
    font-size:.83em;
}

h6 {
    font-weight:bold;
    margin-top:2.33em;
    margin-bottom:2.33em;
    font-size:.67em;
}

br {
display:inline-block;
}

br[clear="all"]
{
clear:both;
}

br[clear="left"]
{
clear:left;
}

br[clear="right"]
{
clear:right;
}

span {
    display:inline
}

img {
display: inline-block;
}

img[align="right"]
{
    float: right;
}

img[align="left"]
{
    float: left;
}

hr {
display: block;
    margin-top: 0.5em;
    margin-bottom: 0.5em;
    margin-left: auto;
    margin-right: auto;
    border-style: inset;
    border-width: 1px
}


/***************** TABLES ********************/

table {
display: table;
    border-collapse: separate;
    border-spacing: 2px;
    border-top-color:gray;
    border-left-color:gray;
    border-bottom-color:black;
    border-right-color:black;
}

tbody, tfoot, thead {
display:table-row-group;
    vertical-align:middle;
}

tr {
display: table-row;
    vertical-align: inherit;
    border-color: inherit;
}

td, th {
display: table-cell;
    vertical-align: inherit;
    border-width:1px;
padding:1px;
}

th {
    font-weight: bold;
}

table[border] {
    border-style:solid;
}

table[border|=0] {
    border-style:none;
}

table[border] td, table[border] th {
    border-style:solid;
    border-top-color:black;
    border-left-color:black;
    border-bottom-color:gray;
    border-right-color:gray;
}

table[border|=0] td, table[border|=0] th {
    border-style:none;
}

caption {
display: table-caption;
}

td[nowrap], th[nowrap] {
    white-space:nowrap;
}

tt, code, kbd, samp {
    font-family: monospace
}
pre, xmp, plaintext, listing {
display: block;
    font-family: monospace;
    white-space: pre;
margin: 1em 0
}

/***************** LISTS ********************/

ul, menu, dir {
display: block;
    list-style-type: disc;
    margin-top: 1em;
    margin-bottom: 1em;
    margin-left: 0;
    margin-right: 0;
    padding-left: 40px
}

ol {
display: block;
    list-style-type: decimal;
    margin-top: 1em;
    margin-bottom: 1em;
    margin-left: 0;
    margin-right: 0;
    padding-left: 40px
}

li {
display: list-item;
}

ul ul, ol ul {
    list-style-type: circle;
}

ol ol ul, ol ul ul, ul ol ul, ul ul ul {
    list-style-type: square;
}

dd {
display: block;
    margin-left: 40px;
}

dl {
display: block;
    margin-top: 1em;
    margin-bottom: 1em;
    margin-left: 0;
    margin-right: 0;
}

dt {
display: block;
}

ol ul, ul ol, ul ul, ol ol {
    margin-top: 0;
    margin-bottom: 0
}

blockquote {
display: block;
    margin-top: 1em;
    margin-bottom: 1em;
    margin-left: 40px;
    margin-left: 40px;
}

/*********** FORM ELEMENTS ************/

form {
display: block;
    margin-top: 0em;
}

option {
display: none;
}

input, textarea, keygen, select, button, isindex {
margin: 0em;
color: initial;
    line-height: normal;
    text-transform: none;
    text-indent: 0;
    text-shadow: none;
display: inline-block;
}
input[type="hidden"] {
display: none;
}


article, aside, footer, header, hgroup, nav, section
{
display: block;
}
)RAW";

class QLiteHtmlWidgetPrivate
{
public:
    litehtml::context context;
    QUrl url;
    DocumentContainer documentContainer;
};

QLiteHtmlWidget::QLiteHtmlWidget(QWidget *parent)
    : QAbstractScrollArea(parent)
    , d(new QLiteHtmlWidgetPrivate)
{
    setMouseTracking(true);
    horizontalScrollBar()->setSingleStep(kScrollBarStep);
    verticalScrollBar()->setSingleStep(kScrollBarStep);

    d->documentContainer.setCursorCallback([this](const QCursor &c) { viewport()->setCursor(c); });
    d->documentContainer.setPaletteCallback([this] { return palette(); });
    d->documentContainer.setLinkCallback([this](const QUrl &url) {
        QUrl fullUrl = url;
        if (url.isRelative() && url.path(QUrl::FullyEncoded).isEmpty()) { // fragment/anchor only
            fullUrl = d->url;
            fullUrl.setFragment(url.fragment(QUrl::FullyEncoded));
        }
        // delay because document may not be changed directly during this callback
        QTimer::singleShot(0, this, [this, fullUrl] { emit linkClicked(fullUrl); });
    });

    // TODO adapt mastercss to palette (default text & background color)
    d->context.load_master_stylesheet(mastercss);
}

QLiteHtmlWidget::~QLiteHtmlWidget()
{
    delete d;
}

void QLiteHtmlWidget::setUrl(const QUrl &url)
{
    d->url = url;
    QUrl urlWithoutAnchor = url;
    urlWithoutAnchor.setFragment({});
    const QString urlString = urlWithoutAnchor.toString(QUrl::None);
    const int lastSlash = urlString.lastIndexOf('/');
    const QString baseUrl = lastSlash >= 0 ? urlString.left(lastSlash) : urlString;
    d->documentContainer.set_base_url(baseUrl.toUtf8().constData());
}

QUrl QLiteHtmlWidget::url() const
{
    return d->url;
}

void QLiteHtmlWidget::setHtml(const QString &content)
{
    litehtml::document::ptr doc = litehtml::document::createFromUTF8(content.toUtf8().constData(),
                                                                     &d->documentContainer,
                                                                     &d->context);
    d->documentContainer.setDocument(doc);
    verticalScrollBar()->setValue(0);
    horizontalScrollBar()->setValue(0);
    render();
}

QString QLiteHtmlWidget::title() const
{
    return d->documentContainer.caption();
}

void QLiteHtmlWidget::setDefaultFont(const QFont &font)
{
    d->documentContainer.setDefaultFont(font);
    render();
}

QFont QLiteHtmlWidget::defaultFont() const
{
    return d->documentContainer.defaultFont();
}

void QLiteHtmlWidget::scrollToAnchor(const QString &name)
{
    if (!d->documentContainer.document())
        return;
    horizontalScrollBar()->setValue(0);
    if (name.isEmpty()) {
        verticalScrollBar()->setValue(0);
        return;
    }
    litehtml::element::ptr element = d->documentContainer.document()->root()->select_one(
        QString("#%1").arg(name).toStdString());
    if (!element) {
        element = d->documentContainer.document()->root()->select_one(
            QString("[name=%1]").arg(name).toStdString());
    }
    if (element) {
        const int y = element->get_placement().y;
        verticalScrollBar()->setValue(std::min(y, verticalScrollBar()->maximum()));
    }
}

void QLiteHtmlWidget::setResourceHandler(const QLiteHtmlWidget::ResourceHandler &handler)
{
    d->documentContainer.setDataCallback(handler);
}

QString QLiteHtmlWidget::selectedText() const
{
    return d->documentContainer.selectedText();
}

void QLiteHtmlWidget::paintEvent(QPaintEvent *event)
{
    if (!d->documentContainer.document())
        return;
    d->documentContainer.setScrollPosition(scrollPosition());
    const QPoint pos = -scrollPosition();
    const QRect r = event->rect();
    const litehtml::position clip = {r.x(), r.y(), r.width(), r.height()};
    QPainter p(viewport());
    d->documentContainer.document()->draw(reinterpret_cast<litehtml::uint_ptr>(&p),
                                          pos.x(),
                                          pos.y(),
                                          &clip);
}

void QLiteHtmlWidget::resizeEvent(QResizeEvent *event)
{
    QAbstractScrollArea::resizeEvent(event);
    render();
}

void QLiteHtmlWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint viewportPos;
    QPoint pos;
    htmlPos(event->pos(), &viewportPos, &pos);
    for (const QRect &r : d->documentContainer.mouseMoveEvent(pos, viewportPos))
        viewport()->update(r.translated(-scrollPosition()));
}

void QLiteHtmlWidget::mousePressEvent(QMouseEvent *event)
{
    QPoint viewportPos;
    QPoint pos;
    htmlPos(event->pos(), &viewportPos, &pos);
    for (const QRect &r : d->documentContainer.mousePressEvent(pos, viewportPos, event->button()))
        viewport()->update(r.translated(-scrollPosition()));
}

void QLiteHtmlWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint viewportPos;
    QPoint pos;
    htmlPos(event->pos(), &viewportPos, &pos);
    for (const QRect &r : d->documentContainer.mouseReleaseEvent(pos, viewportPos, event->button()))
        viewport()->update(r.translated(-scrollPosition()));
}

void QLiteHtmlWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPoint viewportPos;
    QPoint pos;
    htmlPos(event->pos(), &viewportPos, &pos);
    for (const QRect &r :
         d->documentContainer.mouseDoubleClickEvent(pos, viewportPos, event->button())) {
        viewport()->update(r.translated(-scrollPosition()));
    }
}

void QLiteHtmlWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    for (const QRect &r : d->documentContainer.leaveEvent())
        viewport()->update(r.translated(-scrollPosition()));
}

void QLiteHtmlWidget::render()
{
    if (!d->documentContainer.document())
        return;
    const int scrollbarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
    const int w = width() - scrollbarWidth - 2;
    d->documentContainer.render(w, viewport()->height());
    horizontalScrollBar()->setPageStep(viewport()->width());
    horizontalScrollBar()
        ->setRange(0, std::max(0, d->documentContainer.document()->width() - viewport()->width()));
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0,
                                  std::max(0,
                                           d->documentContainer.document()->height()
                                               - viewport()->height()));
    viewport()->update();
}

QPoint QLiteHtmlWidget::scrollPosition() const
{
    return {horizontalScrollBar()->value(), verticalScrollBar()->value()};
}

void QLiteHtmlWidget::htmlPos(const QPoint &pos, QPoint *viewportPos, QPoint *htmlPos) const
{
    *viewportPos = viewport()->mapFromParent(pos);
    *htmlPos = *viewportPos + scrollPosition();
}
