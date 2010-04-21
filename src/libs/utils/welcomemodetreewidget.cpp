/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "welcomemodetreewidget.h"

#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QAction>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMouseEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QImage>

enum { leftContentsMargin = 2,
       topContentsMargin = 2,
       bottomContentsMargin = 1,
       pixmapWidth = 24 };

namespace Utils {

WelcomeModeLabel::WelcomeModeLabel(QWidget *parent) :
    QLabel(parent), m_unused(0)
{
    // Bold/enlarged font slightly gray. Force color on by stylesheet as it is used
    // as a child of widgets that have stylesheets.
    QFont f = font();
#ifndef Q_OS_WIN
    f.setWeight(QFont::DemiBold);
#endif
    f.setPointSizeF(f.pointSizeF() * 1.2);
    setFont(f);
    setStyleSheet(QLatin1String("color : rgb(85, 85, 85);"));
    setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
}

WelcomeModeLabel::~WelcomeModeLabel()
{
}

// NewsLabel for the WelcomeModeTreeWidget:
// Shows a news article as "Bold Title!\nElided Start of article...."
// Adapts the eliding when resizing.
class NewsLabel : public QLabel {
public:
    explicit NewsLabel(const QString &htmlTitle,
                       const QString &htmlText,
                       QWidget *parent = 0);

    virtual void resizeEvent(QResizeEvent *);

private:
    const QString m_title;
    const QString m_text;
    const int m_minWidth;

    int m_lastWidth;
    int m_contentsMargin;
};

NewsLabel::NewsLabel(const QString &htmlTitle,
                   const QString &htmlText,
                   QWidget *parent) :
    QLabel(parent),
    m_title(htmlTitle),
    m_text(htmlText),
    m_minWidth(100),
    m_lastWidth(-1)
{
    int dummy, left, right;
    getContentsMargins(&left, &dummy, &right, &dummy);
    m_contentsMargin = left + right;
}

void NewsLabel::resizeEvent(QResizeEvent *e)
{
    enum { epsilon = 10 };
    QLabel::resizeEvent(e);
    // Resized: Adapt to new size (if it is >  m_minWidth)
    const int newWidth = qMax(e->size().width() - m_contentsMargin, m_minWidth) - epsilon;
    if (newWidth == m_lastWidth)
        return;
    m_lastWidth = newWidth;
    QFont f = font();
    const QString elidedText = QFontMetrics(f).elidedText(m_text, Qt::ElideRight, newWidth);
    f.setBold(true);
    const QString elidedTitle = QFontMetrics(f).elidedText(m_title, Qt::ElideRight, newWidth);
    QString labelText = QLatin1String("<b>");
    labelText += elidedTitle;
    labelText += QLatin1String("</b><br /><font color='gray'>");
    labelText += elidedText;
    labelText += QLatin1String("</font>");
    setText(labelText);
}

// Item label of WelcomeModeTreeWidget: Horizontal widget consisting
// of arrow and clickable label.
class WelcomeModeItemWidget : public QWidget {
    Q_OBJECT
public:
    // Normal items
    explicit WelcomeModeItemWidget(const QPixmap &pix,
                                   const QString &text,
                                   const QString &tooltip,
                                   const QString &data,
                                   QWidget *parent = 0);
    // News items with title and start of article (see NewsLabel)
    explicit WelcomeModeItemWidget(const QPixmap &pix,
                                   QString htmlTitle,
                                   QString htmlText,
                                   const QString &tooltip,
                                   const QString &data,
                                   QWidget *parent = 0);

signals:
    void clicked(const QString &);

protected:
    virtual void mousePressEvent(QMouseEvent *);

private:
    static inline void initLabel(QLabel *);

    void init(const QPixmap &pix, QLabel *itemLabel, const QString &tooltip);

    const QString m_data;
};

WelcomeModeItemWidget::WelcomeModeItemWidget(const QPixmap &pix,
                                             const QString &text,
                                             const QString &tooltipText,
                                             const QString &data,
                                             QWidget *parent) :
    QWidget(parent),
    m_data(data)
{
    QLabel *label = new QLabel(text);
    WelcomeModeItemWidget::initLabel(label);
    init(pix, label, tooltipText);
}

static inline void fixHtml(QString &s)
{
    s.replace(QLatin1String("&#8217;"), QString(QLatin1Char('\''))); // Quote
    s.replace(QLatin1Char('\n'), QLatin1Char(' '));
}

WelcomeModeItemWidget::WelcomeModeItemWidget(const QPixmap &pix,
                                             QString title,
                                             QString text,
                                             const QString &tooltipText,
                                             const QString &data,
                                             QWidget *parent) :
    QWidget(parent),
    m_data(data)
{
    fixHtml(text);
    fixHtml(title);
    QLabel *newsLabel = new NewsLabel(title, text);
    WelcomeModeItemWidget::initLabel(newsLabel);
    init(pix, newsLabel, tooltipText);
}

void WelcomeModeItemWidget::initLabel(QLabel *label)
{
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    label->setCursor(QCursor(Qt::PointingHandCursor));
}

void WelcomeModeItemWidget::init(const QPixmap &pix, QLabel *itemLabel,
                                 const QString &tooltipText)
{
    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    hBoxLayout->setContentsMargins(topContentsMargin, leftContentsMargin,
                                   0, bottomContentsMargin);

    QLabel *pxLabel = new QLabel;
    QPixmap pixmap = pix;
    if (layoutDirection() == Qt::RightToLeft){
        QImage image = pixmap.toImage();
        pixmap = QPixmap::fromImage(image.mirrored(1, 0));
    }
    pxLabel->setPixmap(pixmap);

    pxLabel->setFixedWidth(pixmapWidth);
    hBoxLayout->addWidget(pxLabel);

    hBoxLayout->addWidget(itemLabel);
    if (!tooltipText.isEmpty()) {
        setToolTip(tooltipText);
        pxLabel->setToolTip(tooltipText);
        itemLabel->setToolTip(tooltipText);
    }
    setLayout(hBoxLayout);
}

void WelcomeModeItemWidget::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    emit clicked(m_data);
}

// ----------------- WelcomeModeTreeWidget
struct WelcomeModeTreeWidgetPrivate
{
    WelcomeModeTreeWidgetPrivate();

    const QPixmap bullet;
    QVBoxLayout *layout;
    QVBoxLayout *itemLayout;
};

WelcomeModeTreeWidgetPrivate::WelcomeModeTreeWidgetPrivate() :
    bullet(QLatin1String(":/welcome/images/list_bullet_arrow.png")),
    layout(new QVBoxLayout),
    itemLayout(new QVBoxLayout)
{
    layout->setMargin(0);
}

WelcomeModeTreeWidget::WelcomeModeTreeWidget(QWidget *parent) :
        QWidget(parent), m_d(new WelcomeModeTreeWidgetPrivate)
{
    setLayout(m_d->layout);
    m_d->layout->addLayout(m_d->itemLayout);
    m_d->layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
}

WelcomeModeTreeWidget::~WelcomeModeTreeWidget()
{
    delete m_d;
}

void WelcomeModeTreeWidget::addItem(const QString &label, const QString &data, const QString &toolTip)
{
    addItemWidget(new WelcomeModeItemWidget(m_d->bullet, label, toolTip, data));
}

void WelcomeModeTreeWidget::addNewsItem(const QString &title,
                                        const QString &description,
                                        const QString &link)
{
    addItemWidget(new WelcomeModeItemWidget(m_d->bullet, title, description, link, link));
}

void WelcomeModeTreeWidget::addItemWidget(WelcomeModeItemWidget *w)
{
    connect(w, SIGNAL(clicked(QString)), this, SIGNAL(activated(QString)));
    m_d->itemLayout->addWidget(w);
}

void WelcomeModeTreeWidget::clear()
{
    for (int i = m_d->itemLayout->count() - 1; i >= 0; i--) {
        QLayoutItem *item = m_d->itemLayout->takeAt(i);
        delete item->widget();
        delete item;
    }
}

} // namespace Utils

#include "welcomemodetreewidget.moc"
