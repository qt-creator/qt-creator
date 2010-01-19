#include "targetselector.h"

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QFontMetrics>

static const int TARGET_WIDTH = 109;
static const int TARGET_HEIGHT = 43;
static const int ADDBUTTON_WIDTH = 27;

using namespace ProjectExplorer::Internal;

TargetSelector::TargetSelector(QWidget *parent) :
    QWidget(parent),
    m_currentTargetIndex(-1)
{
        QFont f = font();
        f.setPixelSize(10);
        f.setBold(true);
        setFont(f);
}

void TargetSelector::addTarget(const QString &name)
{
    Target target;
    target.name = name;
    target.currentSubIndex = 0;
    m_targets.append(target);
    if (m_currentTargetIndex == -1)
        m_currentTargetIndex = m_targets.size() - 1;
    update();
}

void TargetSelector::removeTarget(int index)
{
    m_targets.removeAt(index);
    update();
}

TargetSelector::Target TargetSelector::targetAt(int index) const
{
    return m_targets.at(index);
}

QSize TargetSelector::minimumSizeHint() const
{
    return QSize((TARGET_WIDTH + 1) * m_targets.size() + ADDBUTTON_WIDTH + 2, TARGET_HEIGHT + 2);
}

void TargetSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->x() > (TARGET_WIDTH + 1) * m_targets.size()) {
        // check for add button
        event->accept();
        emit addButtonClicked();
    } else {
        // find the clicked target button
        int x = 1;
        int index;
        for (index = 0; index < m_targets.size(); ++index) {
            if (event->x() <= x) {
                break;
            }
            x += TARGET_WIDTH + 1;
        }
        --index;
        if (index >= 0 && index < m_targets.size()) {
            // handle clicked target
            // check if user clicked on Build or Run
            if (event->y() > TARGET_HEIGHT * 3/5) {
                if ((event->x() - (TARGET_WIDTH + 1) * index) - 2 > TARGET_WIDTH / 2) {
                    m_targets[index].currentSubIndex = 1;
                } else {
                    m_targets[index].currentSubIndex = 0;
                }
            }
            m_currentTargetIndex = index;
            //TODO don't emit if nothing changed!
            update();
            emit currentIndexChanged(m_currentTargetIndex, m_targets.at(m_currentTargetIndex).currentSubIndex);
        } else {
            event->ignore();
        }
    }
}

void TargetSelector::paintEvent(QPaintEvent *event)
{
    static QPixmap unselected(":/projectexplorer/images/targetunselected.png");
    static QPixmap runselected(":/projectexplorer/images/targetrunselected.png");
    static QPixmap buildselected(":/projectexplorer/images/targetbuildselected.png");
    static QPixmap targetaddbutton(":/projectexplorer/images/targetaddbutton.png");
    Q_UNUSED(event)

    QPainter p(this);
    p.setPen(QColor(89, 89, 89));
    QSize size = minimumSizeHint();
    //draw frame
    p.drawLine(1, 0, size.width() - 2, 0);
    p.drawLine(1, size.height() - 1, size.width() - 2, size.height() - 1);
    p.drawLine(0, 1, 0, size.height() - 2);
    p.drawLine(size.width() - 1, 1, size.width() - 1, size.height() - 2);
    //draw targets
    int x = 1;
    int index = 0;
    QFontMetrics fm(font());
    foreach (const Target &target, m_targets) {
        QPixmap *pixmap = &unselected;
        if (index == m_currentTargetIndex) {
            p.setPen(QColor(255, 255, 255));
            if (target.currentSubIndex == 0) {
                pixmap = &buildselected;
            } else {
                pixmap = &runselected;
            }
        } else {
            p.setPen(QColor(0, 0, 0));
        }
        p.drawPixmap(x, 1, *pixmap);
        p.drawText(x + (TARGET_WIDTH - fm.width(target.name))/2 + 1, 7 + fm.ascent(),
            target.name);
        x += TARGET_WIDTH;
        p.drawLine(x, 1, x, TARGET_HEIGHT);
        ++x;
        ++index;
    }
    // draw add button
    p.drawPixmap(x, 1, targetaddbutton);
}
