#include "doubletabwidget.h"
#include "ui_doubletabwidget.h"

#include <utils/stylehelper.h>

#include <QtCore/QRect>
#include <QtGui/QPainter>
#include <QtGui/QFont>
#include <QtGui/QMouseEvent>
#include <QtGui/QMenu>

#include <QtDebug>

using namespace ProjectExplorer::Internal;

static const int MIN_LEFT_MARGIN = 50;
static const int MARGIN = 12;
static const int OTHER_HEIGHT = 38;
static const int SELECTION_IMAGE_WIDTH = 10;
static const int SELECTION_IMAGE_HEIGHT = 20;
static const int OVERFLOW_DROPDOWN_WIDTH = Utils::StyleHelper::navigationWidgetHeight();

static void drawFirstLevelSeparator(QPainter *painter, QPoint top, QPoint bottom)
{
    QLinearGradient grad(top, bottom);
    grad.setColorAt(0, QColor(255, 255, 255, 20));
    grad.setColorAt(0.4, QColor(255, 255, 255, 60));
    grad.setColorAt(0.7, QColor(255, 255, 255, 50));
    grad.setColorAt(1, QColor(255, 255, 255, 40));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top, bottom);
    grad.setColorAt(0, QColor(0, 0, 0, 30));
    grad.setColorAt(0.4, QColor(0, 0, 0, 70));
    grad.setColorAt(0.7, QColor(0, 0, 0, 70));
    grad.setColorAt(1, QColor(0, 0, 0, 40));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top - QPoint(1,0), bottom - QPoint(1,0));
}

static void drawSecondLevelSeparator(QPainter *painter, QPoint top, QPoint bottom)
{
    QLinearGradient grad(top, bottom);
    grad.setColorAt(0, QColor(255, 255, 255, 0));
    grad.setColorAt(0.4, QColor(255, 255, 255, 100));
    grad.setColorAt(0.7, QColor(255, 255, 255, 100));
    grad.setColorAt(1, QColor(255, 255, 255, 0));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top, bottom);
    grad.setColorAt(0, QColor(0, 0, 0, 0));
    grad.setColorAt(0.4, QColor(0, 0, 0, 100));
    grad.setColorAt(0.7, QColor(0, 0, 0, 100));
    grad.setColorAt(1, QColor(0, 0, 0, 0));
    painter->setPen(QPen(grad, 0));
    painter->drawLine(top - QPoint(1,0), bottom - QPoint(1,0));
}

DoubleTabWidget::DoubleTabWidget(QWidget *parent) :
    QWidget(parent),
    m_left(QLatin1String(":/projectexplorer/images/leftselection.png")),
    m_mid(QLatin1String(":/projectexplorer/images/midselection.png")),
    m_right(QLatin1String(":/projectexplorer/images/rightselection.png")),
    ui(new Ui::DoubleTabWidget),
    m_currentIndex(-1),
    m_lastVisibleIndex(-1)
{
    ui->setupUi(this);
}

DoubleTabWidget::~DoubleTabWidget()
{
    delete ui;
}

int DoubleTabWidget::currentIndex() const
{
    return m_currentIndex;
}

void DoubleTabWidget::setCurrentIndex(int index)
{
    Q_ASSERT(index < m_tabs.size());
    if (index == m_currentIndex)
        return;
    m_currentIndex = index;
    emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
    update();
}

void DoubleTabWidget::setTitle(const QString &title)
{
    m_title = title;
    update();
}

QSize DoubleTabWidget::minimumSizeHint() const
{
    return QSize(0, Utils::StyleHelper::navigationWidgetHeight() + OTHER_HEIGHT + 1);
}

void DoubleTabWidget::addTab(const QString &name, const QStringList &subTabs)
{
    Tab tab;
    tab.name = name;
    tab.subTabs = subTabs;
    tab.currentSubTab = tab.subTabs.isEmpty() ? -1 : 0;
    m_tabs.append(tab);
    if (m_currentIndex == -1) {
        m_currentIndex = m_tabs.size()-1;
        emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
    }
    update();
}

void DoubleTabWidget::insertTab(int index, const QString &name, const QStringList &subTabs)
{
    Tab tab;
    tab.name = name;
    tab.subTabs = subTabs;
    tab.currentSubTab = tab.subTabs.isEmpty() ? -1 : 0;
    m_tabs.insert(index, tab);
    if (m_currentIndex == -1) {
        m_currentIndex = index;
        emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
    } else if (m_currentIndex >= index) {
        ++m_currentIndex;
        emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
    }
    update();
}

void DoubleTabWidget::removeTab(int index)
{
    m_tabs.removeAt(index);
    if (index <= m_currentIndex) {
        --m_currentIndex;
        if (m_currentIndex < 0 && m_tabs.size() > 0)
            m_currentIndex = 0;
        if (m_currentIndex < 0) {
            emit currentIndexChanged(-1, -1);
        } else {
            emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
        }
    }
    update();
}

int DoubleTabWidget::tabCount() const
{
    return m_tabs.size();
}

void DoubleTabWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->y() < Utils::StyleHelper::navigationWidgetHeight()) {
        int eventX = event->x();
        // clicked on the top level part of the bar
        QFontMetrics fm(font());
        int x = m_title.isEmpty() ? 0 :
                2 * MARGIN + qMax(fm.width(m_title), MIN_LEFT_MARGIN);

        if (eventX <= x)
            return;
        int i;
        for (i = 0; i <= m_lastVisibleIndex; ++i) {
            int otherX = x + 2 * MARGIN + fm.width(m_tabs.at(
                    m_currentTabIndices.at(i)).name);
            if (eventX > x && eventX < otherX) {
                break;
            }
            x = otherX;
        }
        if (i <= m_lastVisibleIndex) {
            if (m_currentIndex != m_currentTabIndices.at(i)) {
                m_currentIndex = m_currentTabIndices.at(i);
                update();
                emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
            }
            event->accept();
            return;
        } else if (m_lastVisibleIndex < m_tabs.size() - 1) {
            // handle overflow menu
            if (eventX > x && eventX < x + OVERFLOW_DROPDOWN_WIDTH) {
                QMenu overflowMenu;
                QList<QAction *> actions;
                for (int i = m_lastVisibleIndex + 1; i < m_tabs.size(); ++i) {
                    actions << overflowMenu.addAction(m_tabs.at(m_currentTabIndices.at(i)).name);
                }
                if (QAction *action = overflowMenu.exec(mapToGlobal(QPoint(x+1, 1)))) {
                    int index = m_currentTabIndices.at(actions.indexOf(action) + m_lastVisibleIndex + 1);
                    if (m_currentIndex != index) {
                        m_currentIndex = index;
                        update();
                        emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
                    }
                }
            }
        }
    } else if (event->y() < Utils::StyleHelper::navigationWidgetHeight() + OTHER_HEIGHT) {
        int diff = (OTHER_HEIGHT - SELECTION_IMAGE_HEIGHT) / 2;
        if (event->y() < Utils::StyleHelper::navigationWidgetHeight() + diff
                || event->y() > Utils::StyleHelper::navigationWidgetHeight() + OTHER_HEIGHT - diff)
            return;
        if (m_currentIndex == -1)
            return;
        Tab currentTab = m_tabs.at(m_currentIndex);
        QStringList subTabs = currentTab.subTabs;
        if (subTabs.isEmpty())
            return;
        int eventX = event->x();
        QFontMetrics fm(font());
        // clicked on the lower level part of the bar
        int x = MARGIN;
        int i;
        for (i = 0; i < subTabs.size(); ++i) {
            int otherX = x + 2 * SELECTION_IMAGE_WIDTH + fm.width(subTabs.at(i));
            if (eventX > x && eventX < otherX) {
                break;
            }
            x = otherX + 2 * MARGIN;
        }
        if (i < subTabs.size()) {
            if (m_tabs[m_currentIndex].currentSubTab != i) {
                m_tabs[m_currentIndex].currentSubTab = i;
                update();
            }
            event->accept();
            emit currentIndexChanged(m_currentIndex, m_tabs.at(m_currentIndex).currentSubTab);
            return;
        }
    }
    event->ignore();
}

void DoubleTabWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    QRect r = rect();

    // draw top level tab bar
    r.setHeight(Utils::StyleHelper::navigationWidgetHeight());

    QPoint offset = window()->mapToGlobal(QPoint(0, 0)) - mapToGlobal(r.topLeft());
    QRect gradientSpan = QRect(offset, window()->size());
    Utils::StyleHelper::horizontalGradient(&painter, gradientSpan, r);

    painter.setPen(Utils::StyleHelper::borderColor());

    QColor lighter(255, 255, 255, 40);
    painter.drawLine(r.bottomLeft(), r.bottomRight());
    painter.setPen(lighter);
    painter.drawLine(r.topLeft(), r.topRight());

    QFontMetrics fm(font());
    int baseline = (r.height() + fm.ascent()) / 2 - 1;

    // top level title
    if (!m_title.isEmpty()) {
        painter.setPen(Utils::StyleHelper::panelTextColor());
        painter.drawText(MARGIN, baseline, m_title);
    }

    QLinearGradient grad(QPoint(0, 0), QPoint(0, r.height() + OTHER_HEIGHT - 1));
    grad.setColorAt(0, QColor(247, 247, 247));
    grad.setColorAt(1, QColor(205, 205, 205));

    // draw background of second bar
    painter.fillRect(QRect(0, r.height(), r.width(), OTHER_HEIGHT), grad);
    painter.setPen(Qt::black);
    painter.drawLine(0, r.height() + OTHER_HEIGHT,
                     r.width(), r.height() + OTHER_HEIGHT);
    painter.setPen(Qt::white);
    painter.drawLine(0, r.height(),
                     r.width(), r.height());

    // top level tabs
    int x = m_title.isEmpty() ? 0 :
            2 * MARGIN + qMax(fm.width(m_title), MIN_LEFT_MARGIN);

    // calculate sizes
    QList<int> nameWidth;
    int width = x;
    int indexSmallerThanOverflow = -1;
    int indexSmallerThanWidth = -1;
    for (int i = 0; i < m_tabs.size(); ++i) {
        const Tab &tab = m_tabs.at(i);
        int w = fm.width(tab.name);
        nameWidth << w;
        width += 2 * MARGIN + w;
        if (width < r.width())
            indexSmallerThanWidth = i;
        if (width < r.width() - OVERFLOW_DROPDOWN_WIDTH)
            indexSmallerThanOverflow = i;
    }
    m_lastVisibleIndex = -1;
    m_currentTabIndices.resize(m_tabs.size());
    if (indexSmallerThanWidth == m_tabs.size() - 1) {
        // => everything fits
        for (int i = 0; i < m_tabs.size(); ++i)
            m_currentTabIndices[i] = i;
        m_lastVisibleIndex = m_tabs.size()-1;
    } else {
        // => we need the overflow thingy
        if (m_currentIndex <= indexSmallerThanOverflow) {
            // easy going, simply draw everything that fits
            for (int i = 0; i < m_tabs.size(); ++i)
                m_currentTabIndices[i] = i;
            m_lastVisibleIndex = indexSmallerThanOverflow;
        } else {
            // now we need to put the current tab into
            // visible range. for that we need to find the place
            // to put it, so it fits
            width = x;
            int index = 0;
            bool handledCurrentIndex = false;
            for (int i = 0; i < m_tabs.size(); ++i) {
                if (index != m_currentIndex) {
                    if (!handledCurrentIndex) {
                        // check if enough room for current tab after this one
                        if (width + 2 * MARGIN + nameWidth.at(index)
                                + 2 * MARGIN + nameWidth.at(m_currentIndex)
                                < r.width() - OVERFLOW_DROPDOWN_WIDTH) {
                            m_currentTabIndices[i] = index;
                            ++index;
                            width += 2 * MARGIN + nameWidth.at(index);
                        } else {
                            m_currentTabIndices[i] = m_currentIndex;
                            handledCurrentIndex = true;
                            m_lastVisibleIndex = i;
                        }
                    } else {
                        m_currentTabIndices[i] = index;
                        ++index;
                    }
                } else {
                    ++index;
                    --i;
                }
            }
        }
    }

    // actually draw top level tabs
    for (int i = 0; i <= m_lastVisibleIndex; ++i) {
        int actualIndex = m_currentTabIndices.at(i);
        Tab tab = m_tabs.at(actualIndex);
        if (actualIndex == m_currentIndex) {
            painter.setPen(Utils::StyleHelper::borderColor());
            painter.drawLine(x - 1, 0, x - 1, r.height() - 1);
            painter.fillRect(QRect(x, 0,
                                   2 * MARGIN + fm.width(tab.name),
                                   r.height() + 1),
                             grad);

            if (actualIndex != 0) {
                painter.setPen(QColor(255, 255, 255, 170));
                painter.drawLine(x, 0, x, r.height());
            }
            x += MARGIN;
            painter.setPen(Qt::black);
            painter.drawText(x, baseline, tab.name);
            x += nameWidth.at(actualIndex);
            x += MARGIN;
            painter.setPen(Utils::StyleHelper::borderColor());
            painter.drawLine(x, 0, x, r.height() - 1);
            painter.setPen(QColor(0, 0, 0, 20));
            painter.drawLine(x + 1, 0, x + 1, r.height() - 1);
            painter.setPen(QColor(255, 255, 255, 170));
            painter.drawLine(x - 1, 0, x - 1, r.height());
        } else {
            if (i == 0)
                drawFirstLevelSeparator(&painter, QPoint(x, 0), QPoint(x, r.height()-1));
            x += MARGIN;
            painter.setPen(Utils::StyleHelper::panelTextColor());
            painter.drawText(x + 1, baseline, tab.name);
            x += nameWidth.at(actualIndex);
            x += MARGIN;
            drawFirstLevelSeparator(&painter, QPoint(x, 0), QPoint(x, r.height()-1));
        }
    }

    // draw overflow button
    if (m_lastVisibleIndex < m_tabs.size() - 1) {
        QStyleOption opt;
        opt.rect = QRect(x, 0, OVERFLOW_DROPDOWN_WIDTH - 1, r.height() - 1);
        style()->drawPrimitive(QStyle::PE_IndicatorArrowDown,
                               &opt, &painter, this);
        drawFirstLevelSeparator(&painter, QPoint(x + OVERFLOW_DROPDOWN_WIDTH, 0),
                                QPoint(x + OVERFLOW_DROPDOWN_WIDTH, r.height()-1));
    }

    // second level tabs
    if (m_currentIndex != -1) {
        int y = r.height() + (OTHER_HEIGHT - m_left.height()) / 2.;
        int imageHeight = m_left.height();
        Tab currentTab = m_tabs.at(m_currentIndex);
        QStringList subTabs = currentTab.subTabs;
        x = 0;
        for (int i = 0; i < subTabs.size(); ++i) {
            x += MARGIN;
            int textWidth = fm.width(subTabs.at(i));
            if (currentTab.currentSubTab == i) {
                painter.setPen(Qt::white);
                painter.drawPixmap(x, y, m_left);
                painter.drawPixmap(QRect(x + SELECTION_IMAGE_WIDTH, y,
                                         textWidth, imageHeight),
                                   m_mid, QRect(0, 0, m_mid.width(), m_mid.height()));
                painter.drawPixmap(x + SELECTION_IMAGE_WIDTH + textWidth, y, m_right);
            } else {
                painter.setPen(Qt::black);
            }
            x += SELECTION_IMAGE_WIDTH;
            painter.drawText(x, y + (imageHeight + fm.ascent()) / 2. - 1,
                             subTabs.at(i));
            x += textWidth + SELECTION_IMAGE_WIDTH + MARGIN;
            drawSecondLevelSeparator(&painter, QPoint(x, y), QPoint(x, y + imageHeight));
        }
    }
}

void DoubleTabWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
