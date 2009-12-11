/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#include "canvasframerate.h"

#include <private/qmldebugclient_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qmargins.h>

#include <QtGui/qapplication.h>
#include <QtGui/qpainter.h>
#include <QtGui/qtooltip.h>
#include <QtGui/qslider.h>
#include <QtGui/qscrollbar.h>
#include <QtGui/qspinbox.h>
#include <QtGui/qgroupbox.h>
#include <QtGui/qboxlayout.h>
#include <QtGui/qlabel.h>
#include <QtGui/qlineedit.h>
#include <QtGui/qpushbutton.h>
#include <QtGui/qtabwidget.h>
#include <QtGui/qevent.h>


QT_BEGIN_NAMESPACE

class QLineGraph : public QWidget
{
Q_OBJECT
public:
    QLineGraph(QAbstractSlider *slider, QWidget * = 0);

    void setPosition(int);

public slots:
    void addSample(int, int, int, bool);
    void setResolutionForHeight(int);
    void clear();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void wheelEvent(QWheelEvent *event);

private slots:
    void sliderChanged(int);

private:
    void updateSlider();
    void drawSample(QPainter *, int, const QRect &, QList<QRect> *);
    void drawTime(QPainter *, const QRect &);
    QRect findContainingRect(const QList<QRect> &rects, const QPoint &pos) const;
    struct Sample { 
        int sample[3];
        bool isBreak;
    };
    QList<Sample> _samples;

    QAbstractSlider *slider;
    int position;
    int samplesPerWidth;
    int resolutionForHeight;
    bool ignoreScroll;
    QMargins graphMargins;

    QList<QRect> rectsPaintTime;    // time to do a paintEvent()
    QList<QRect> rectsTimeBetween;  // time between frames
    QRect highlightedBar;
};

QLineGraph::QLineGraph(QAbstractSlider *slider, QWidget *parent)
: QWidget(parent), slider(slider), position(-1), samplesPerWidth(99), resolutionForHeight(50),
  ignoreScroll(false), graphMargins(65, 10, 71, 35)
{
    setMouseTracking(true);

    slider->setMaximum(0);
    slider->setMinimum(0);
    slider->setSingleStep(1);
    
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)));
}

void QLineGraph::sliderChanged(int v)
{
    if (ignoreScroll)
        return;

    if (v == slider->maximum())
        position = -1;
    else
        position = v;
    
    update();
    
    // update highlightedRect
    QPoint pos = mapFromGlobal(QCursor::pos());
    if (geometry().contains(pos)) {
        QMouseEvent *me = new QMouseEvent(QEvent::MouseMove, pos,
                Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::postEvent(this, me);
    }
}

void QLineGraph::clear()
{
    _samples.clear();
    rectsPaintTime.clear();
    rectsTimeBetween.clear();
    highlightedBar = QRect();
    position = -1;

    updateSlider();
    update();
}

void QLineGraph::updateSlider()
{
    ignoreScroll = true;
    slider->setMaximum(qMax(0, _samples.count() - samplesPerWidth - 1));

    if (position == -1) {
        slider->setValue(slider->maximum());
    } else {
        slider->setValue(position);
    }    
    ignoreScroll = false;
}

void QLineGraph::addSample(int a, int b, int d, bool isBreak)
{
    Sample s;
    s.isBreak = isBreak;
    s.sample[0] = a;
    s.sample[1] = b;
    s.sample[2] = d;
    _samples << s;
    updateSlider();
    update();
}

void QLineGraph::setPosition(int p)
{
    sliderChanged(p);
}

void QLineGraph::drawTime(QPainter *p, const QRect &rect)
{
    if (_samples.isEmpty())
        return;

    int first = position;
    if (first == -1)
        first = qMax(0, _samples.count() - samplesPerWidth - 1);
    int last = qMin(_samples.count() - 1, first + samplesPerWidth);

    qreal scaleX = qreal(rect.width()) / qreal(samplesPerWidth);

    int t = 0;

    for (int ii = first; ii <= last; ++ii) {
        int sampleTime = _samples.at(ii).sample[2] / 1000;
        if (sampleTime != t) {

            int xEnd = rect.left() + scaleX * (ii - first);
            p->drawLine(xEnd, rect.bottom(), xEnd, rect.bottom() + 7);

            QRect text(xEnd - 30, rect.bottom() + 10, 60, 30);

            p->drawText(text, Qt::AlignHCenter | Qt::AlignTop, QString::number(_samples.at(ii).sample[2]));

            t = sampleTime;
        }
    }

}

void QLineGraph::drawSample(QPainter *p, int s, const QRect &rect, QList<QRect> *record)
{
    if (_samples.isEmpty())
        return;

    int first = position;
    if (first == -1)
        first = qMax(0, _samples.count() - samplesPerWidth - 1);
    int last = qMin(_samples.count() - 1, first + samplesPerWidth);

    qreal scaleY = qreal(rect.height()) / resolutionForHeight;
    qreal scaleX = qreal(rect.width()) / qreal(samplesPerWidth);

    int xEnd;
    int lastXEnd = rect.left();

    p->save();
    p->setPen(Qt::NoPen);
    for (int ii = first + 1; ii <= last; ++ii) {

        xEnd = rect.left() + scaleX * (ii - first);
        int yEnd = rect.bottom() - _samples.at(ii).sample[s] * scaleY;

        if (!(s == 0 && _samples.at(ii).isBreak)) {
            QRect bar(lastXEnd, yEnd, scaleX, _samples.at(ii).sample[s] * scaleY);
            record->append(bar);
            p->drawRect(bar);
        }

        lastXEnd = xEnd;
    }
    p->restore();
}

void QLineGraph::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r(graphMargins.left(), graphMargins.top(),
            width() - graphMargins.right(), height() - graphMargins.bottom());

    p.save();
    p.rotate(-90);
    p.translate(-r.height()/2 - r.width()/2 - graphMargins.right(), -r.height()/2);
    p.drawText(r, Qt::AlignCenter, tr("Frame rate"));
    p.restore();

    p.setBrush(QColor("lightsteelblue"));
    rectsTimeBetween.clear();
    drawSample(&p, 0, r, &rectsTimeBetween);

    p.setBrush(QColor("pink"));
    rectsPaintTime.clear();
    drawSample(&p, 1, r, &rectsPaintTime);

    if (!highlightedBar.isNull()) {
        p.setBrush(Qt::darkGreen);
        p.drawRect(highlightedBar);
    }

    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    slider->setGeometry(x() + r.x(), slider->y(), r.width(), slider->height());

    for (int ii = 0; ii <= resolutionForHeight; ++ii) {
        int y = 1 + r.bottom() - ii * r.height() / resolutionForHeight;

        if ((ii % 10) == 0) {
            p.drawLine(r.left() - 20, y, r.left(), y);
            QRect text(r.left() - 20 - 53, y - 10, 50, 20);
            p.drawText(text, Qt::AlignRight | Qt::AlignVCenter, QString::number(ii));
        } else {
            p.drawLine(r.left() - 7, y, r.left(), y);
        }
    }

    drawTime(&p, r);
}

void QLineGraph::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();

    QRect rect = findContainingRect(rectsPaintTime, pos);
    if (rect.isNull())
        rect = findContainingRect(rectsTimeBetween, pos);

    if (!highlightedBar.isNull())
        update(highlightedBar.adjusted(-1, -1, 1, 1));
    highlightedBar = rect;

    if (!rect.isNull()) {
        QRect graph(graphMargins.left(), graphMargins.top(),
                    width() - graphMargins.right(), height() - graphMargins.bottom());
        qreal scaleY = qreal(graph.height()) / resolutionForHeight;
        QToolTip::showText(event->globalPos(), QString::number(qRound(rect.height() / scaleY)), this, rect);
        update(rect.adjusted(-1, -1, 1, 1));
    }
}

void QLineGraph::leaveEvent(QEvent *)
{
    if (!highlightedBar.isNull()) {
        QRect bar = highlightedBar.adjusted(-1, -1, 1, 1);
        highlightedBar = QRect();
        update(bar);
    }
}

void QLineGraph::wheelEvent(QWheelEvent *event)
{
    QWheelEvent we(QPoint(0,0), event->delta(), event->buttons(), event->modifiers(), event->orientation());
    QApplication::sendEvent(slider, &we);
}

void QLineGraph::setResolutionForHeight(int resolution)
{
    resolutionForHeight = resolution;
    update();
}

QRect QLineGraph::findContainingRect(const QList<QRect> &rects, const QPoint &pos) const
{
    for (int i=0; i<rects.count(); ++i) {
        if (rects[i].contains(pos))
            return rects[i]; 
    }
    return QRect();
}


class GraphWindow : public QWidget
{
    Q_OBJECT
public:
    GraphWindow(QWidget *parent = 0);

    virtual QSize sizeHint() const;

public slots:
    void addSample(int, int, int, bool);
    void setResolutionForHeight(int);
    void clear();

private:
    QLineGraph *m_graph;
};

GraphWindow::GraphWindow(QWidget *parent)
    : QWidget(parent)
{
    QSlider *scroll = new QSlider(Qt::Horizontal);
    scroll->setFocusPolicy(Qt::WheelFocus);
    m_graph = new QLineGraph(scroll);

    setFocusPolicy(Qt::WheelFocus);
    setFocusProxy(scroll);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 5, 0);
    layout->setSpacing(0);
    layout->addWidget(m_graph, 2);
    layout->addWidget(new QLabel(tr("Total time elapsed (ms)")), 0, Qt::AlignHCenter);
    layout->addWidget(scroll);
}

void GraphWindow::addSample(int a, int b, int d, bool isBreak)
{
    m_graph->addSample(a, b, d, isBreak);
}

void GraphWindow::setResolutionForHeight(int res)
{
    m_graph->setResolutionForHeight(res);
}

void GraphWindow::clear()
{
    m_graph->clear();
}

QSize GraphWindow::sizeHint() const
{
    return QSize(400, 220);
}
    

class CanvasFrameRatePlugin : public QmlDebugClient
{
    Q_OBJECT
public:
    CanvasFrameRatePlugin(QmlDebugConnection *client);

signals:
    void sample(int, int, int, bool);

protected:
    virtual void messageReceived(const QByteArray &);

private:
    int lb;
    int ld;
};

CanvasFrameRatePlugin::CanvasFrameRatePlugin(QmlDebugConnection *client)
: QmlDebugClient(QLatin1String("CanvasFrameRate"), client), lb(-1)
{
}

void CanvasFrameRatePlugin::messageReceived(const QByteArray &data)
{
    QByteArray rwData = data;
    QDataStream stream(&rwData, QIODevice::ReadOnly);

    int b; int c; int d; bool isBreak;
    stream >> b >> c >> d >> isBreak;

    if (lb != -1)
        emit sample(c, lb, ld, isBreak);

    lb = b;
    ld = d;
}

CanvasFrameRate::CanvasFrameRate(QWidget *parent)
: QWidget(parent),
  m_plugin(0)
{
    m_tabs = new QTabWidget(this);

    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->setMargin(0);
    bottom->setSpacing(10);

    m_res = new QSpinBox;
    m_res->setRange(30, 200);
    m_res->setValue(m_res->minimum());
    m_res->setSingleStep(10);
    m_res->setSuffix(QLatin1String("ms"));
    bottom->addWidget(new QLabel(tr("Resolution:")));
    bottom->addWidget(m_res);

    bottom->addStretch();

    m_clearButton = new QPushButton(tr("Clear"));
    connect(m_clearButton, SIGNAL(clicked()), SLOT(clearGraph()));
    bottom->addWidget(m_clearButton);

    QPushButton *pb = new QPushButton(tr("New Graph"), this);
    connect(pb, SIGNAL(clicked()), this, SLOT(newTab()));
    bottom->addWidget(pb);
    
    m_group = new QGroupBox(tr("Enabled"));
    m_group->setCheckable(true);
    m_group->setChecked(false);
    connect(m_group, SIGNAL(toggled(bool)), SLOT(enabledToggled(bool)));

    QVBoxLayout *groupLayout = new QVBoxLayout(m_group);
    groupLayout->setContentsMargins(5, 0, 5, 0);
    groupLayout->setSpacing(2);
    groupLayout->addWidget(m_tabs);
    groupLayout->addLayout(bottom);
    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_group);
    setLayout(layout);
}

void CanvasFrameRate::reset(QmlDebugConnection *conn)
{
    delete m_plugin;
    m_plugin = 0;

    QWidget *w;
    for (int i=0; i<m_tabs->count(); ++i) {
        w = m_tabs->widget(i);
        m_tabs->removeTab(i);
        delete w;
    }

    if (conn) {
        connect(conn, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
                SLOT(connectionStateChanged(QAbstractSocket::SocketState)));
        if (conn->state() == QAbstractSocket::ConnectedState)
            handleConnected(conn);
    }
}

void CanvasFrameRate::connectionStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::UnconnectedState) {
        delete m_plugin;
        m_plugin = 0;
    } else if (state == QAbstractSocket::ConnectedState) {
        handleConnected(qobject_cast<QmlDebugConnection*>(sender()));
    }        
}

void CanvasFrameRate::handleConnected(QmlDebugConnection *conn)
{
    delete m_plugin;
    m_plugin = new CanvasFrameRatePlugin(conn);
    enabledToggled(m_group->isChecked());
    newTab();
}

void CanvasFrameRate::setSizeHint(const QSize &size)
{
    m_sizeHint = size;
}

QSize CanvasFrameRate::sizeHint() const
{
    return m_sizeHint;
}

void CanvasFrameRate::clearGraph()
{
    if (m_tabs->count()) {
        GraphWindow *w = qobject_cast<GraphWindow*>(m_tabs->currentWidget());
        if (w)
            w->clear();
    }
}

void CanvasFrameRate::newTab()
{
    if (!m_plugin)
        return;

    if (m_tabs->count()) {
        QWidget *w = m_tabs->widget(m_tabs->count() - 1);
        QObject::disconnect(m_plugin, SIGNAL(sample(int,int,int,bool)),
                            w, SLOT(addSample(int,int,int,bool)));
    }

    int count = m_tabs->count();

    GraphWindow *graph = new GraphWindow;
    graph->setResolutionForHeight(m_res->value());
    connect(m_plugin, SIGNAL(sample(int,int,int,bool)),
            graph, SLOT(addSample(int,int,int,bool)));
    connect(m_res, SIGNAL(valueChanged(int)),
            graph, SLOT(setResolutionForHeight(int)));

    QString name = QLatin1String("Graph ") + QString::number(count + 1);
    m_tabs->addTab(graph, name);
    m_tabs->setCurrentIndex(count);
}

void CanvasFrameRate::enabledToggled(bool checked)
{
    if (m_plugin)
        static_cast<QmlDebugClient *>(m_plugin)->setEnabled(checked);
}

QT_END_NAMESPACE

#include "canvasframerate.moc"
