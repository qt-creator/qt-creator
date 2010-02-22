#include "targetselector.h"

#include <utils/qtcassert.h>

#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QFontMetrics>

static const int TARGET_WIDTH = 109;
static const int TARGET_HEIGHT = 43;
static const int ADDBUTTON_WIDTH = 27;

using namespace ProjectExplorer::Internal;

TargetSelector::TargetSelector(QWidget *parent) :
    QWidget(parent),
    m_unselected(QLatin1String(":/projectexplorer/images/targetunselected.png")),
    m_runselected(QLatin1String(":/projectexplorer/images/targetrunselected.png")),
    m_buildselected(QLatin1String(":/projectexplorer/images/targetbuildselected.png")),
    m_targetaddbutton(QLatin1String(":/projectexplorer/images/targetaddbutton.png")),
    m_targetremovebutton(QLatin1String(":/projectexplorer/images/targetremovebutton.png")),
    m_currentTargetIndex(-1),
    m_addButtonEnabled(true),
    m_removeButtonEnabled(false)
{
    QFont f = font();
    f.setPixelSize(10);
    f.setBold(true);
    setFont(f);
}

void TargetSelector::addTarget(const QString &name)
{
    insertTarget(m_targets.count(), name);
}

void TargetSelector::insertTarget(int index, const QString &name)
{
    QTC_ASSERT(index >= 0 && index <= m_targets.count(), return);

    Target target;
    target.name = name;
    target.currentSubIndex = 0;
    target.isActive = false;

    m_targets.insert(index, target);

    if (m_currentTargetIndex == -1)
        setCurrentIndex(index);
    update();
}

void TargetSelector::markActive(int index)
{
    QTC_ASSERT(index >= 0 && index < m_targets.count(), return);

    for (int i = 0; i < m_targets.count(); ++i)
        m_targets[i].isActive = (i == index);
    update();
}

void TargetSelector::removeTarget(int index)
{
    QTC_ASSERT(index >= 0 && index < m_targets.count(), return);

    m_targets.removeAt(index);
    if (m_currentTargetIndex > index)
        setCurrentIndex(m_currentTargetIndex - 1);
    if (m_currentTargetIndex == m_targets.count())
        setCurrentIndex(m_currentTargetIndex - 1);
    update();
}

void TargetSelector::setCurrentIndex(int index)
{
    if (index < -1 ||
        index >= m_targets.count() ||
        index == m_currentTargetIndex)
        return;

    if (index == -1 && !m_targets.isEmpty())
        return;

    m_currentTargetIndex = index;

    update();
    emit currentIndexChanged(m_currentTargetIndex,
                             m_currentTargetIndex >= 0 ? m_targets.at(m_currentTargetIndex).currentSubIndex : -1);
}

void TargetSelector::setAddButtonEnabled(bool enabled)
{
    m_addButtonEnabled = enabled;
}

void TargetSelector::setRemoveButtonEnabled(bool enabled)
{
    m_removeButtonEnabled = enabled;
}

void TargetSelector::setCurrentSubIndex(int subindex)
{
    if (subindex < 0 ||
        subindex >= 2 ||
        m_currentTargetIndex < 0 ||
        subindex == m_targets.at(m_currentTargetIndex).currentSubIndex)
        return;
    m_targets[m_currentTargetIndex].currentSubIndex = subindex;

    update();
    emit currentIndexChanged(m_currentTargetIndex,
                             m_targets.at(m_currentTargetIndex).currentSubIndex);
}

TargetSelector::Target TargetSelector::targetAt(int index) const
{
    return m_targets.at(index);
}

bool TargetSelector::isAddButtonEnabled() const
{
    return m_addButtonEnabled;
}

bool TargetSelector::isRemoveButtonEnabled() const
{
    return m_removeButtonEnabled;
}

QSize TargetSelector::minimumSizeHint() const
{
    return QSize((TARGET_WIDTH + 1) * m_targets.size() + (ADDBUTTON_WIDTH + 1) * 2 + 1, TARGET_HEIGHT + 2);
}

void TargetSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->x() < ADDBUTTON_WIDTH) {
        event->accept();
        if (m_removeButtonEnabled)
            emit removeButtonClicked();
    } else if (event->x() > ADDBUTTON_WIDTH + (TARGET_WIDTH + 1) * m_targets.size()) {
        // check for add button
        event->accept();
        if (m_addButtonEnabled)
            emit addButtonClicked();
    } else {
        // find the clicked target button
        int x = ADDBUTTON_WIDTH;
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
                if ((event->x() - (ADDBUTTON_WIDTH + (TARGET_WIDTH + 1) * index)) - 2 > TARGET_WIDTH / 2) {
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
    p.drawPixmap(x, 1, m_targetremovebutton);
    x += m_targetremovebutton.width();
    p.setPen(QColor(0, 0, 0));
    p.drawLine(x, 1, x, TARGET_HEIGHT);
    x += 1;
    foreach (const Target &target, m_targets) {
        const QPixmap *pixmap = &m_unselected;
        if (index == m_currentTargetIndex) {
            p.setPen(QColor(255, 255, 255));
            if (target.currentSubIndex == 0) {
                pixmap = &m_buildselected;
            } else {
                pixmap = &m_runselected;
            }
        } else {
            p.setPen(QColor(0, 0, 0));
        }
        p.drawPixmap(x, 1, *pixmap);
        QString targetName;
        if (target.isActive)
            targetName = QChar('*') + target.name + QChar('*');
        else
            targetName = target.name;
        p.drawText(x + (TARGET_WIDTH - fm.width(targetName))/2 + 1, 7 + fm.ascent(),
            targetName);
        x += TARGET_WIDTH;
        p.drawLine(x, 1, x, TARGET_HEIGHT);
        ++x;
        ++index;
    }
    // draw add button
    p.drawPixmap(x, 1, m_targetaddbutton);
}
