#ifndef TARGETSELECTOR_H
#define TARGETSELECTOR_H

#include <QWidget>

namespace ProjectExplorer {
namespace Internal {

class TargetSelector : public QWidget
{
Q_OBJECT
public:
    struct Target {
        QString name;
        int currentSubIndex;
    };

    explicit TargetSelector(QWidget *parent = 0);

    QSize minimumSizeHint() const;

    Target targetAt(int index) const;
    int targetCount() const { return m_targets.size(); }
    int currentIndex() const { return m_currentTargetIndex; }
    int currentSubIndex() const { return m_targets.at(m_currentTargetIndex).currentSubIndex; }

public slots:
    void addTarget(const QString &name);
    void removeTarget(int index);

signals:
    void addButtonClicked();
    void currentIndexChanged(int targetIndex, int subIndex);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    QList<Target> m_targets;

    int m_currentTargetIndex;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSELECTOR_H
