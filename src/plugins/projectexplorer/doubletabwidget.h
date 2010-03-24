#ifndef DOUBLETABWIDGET_H
#define DOUBLETABWIDGET_H

#include <QtCore/QVector>
#include <QtGui/QWidget>
#include <QtGui/QPixmap>

namespace ProjectExplorer {
namespace Internal {

namespace Ui {
    class DoubleTabWidget;
}

class DoubleTabWidget : public QWidget
{
    Q_OBJECT
public:
    DoubleTabWidget(QWidget *parent = 0);
    ~DoubleTabWidget();

    void setTitle(const QString &title);
    QString title() const { return m_title; }

    void addTab(const QString &name, const QStringList &subTabs);
    void insertTab(int index, const QString &name, const QStringList &subTabs);
    void removeTab(int index);
    int tabCount() const;

    int currentIndex() const;
    void setCurrentIndex(int index);

signals:
    void currentIndexChanged(int index, int subIndex);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void changeEvent(QEvent *e);
    QSize minimumSizeHint() const;

private:
    struct Tab {
        QString name;
        QStringList subTabs;
        int currentSubTab;
    };

    const QPixmap m_left;
    const QPixmap m_mid;
    const QPixmap m_right;

    Ui::DoubleTabWidget *ui;


    QString m_title;
    QList<Tab> m_tabs;
    int m_currentIndex;
    QVector<int> m_currentTabIndices;
    int m_lastVisibleIndex;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // DOUBLETABWIDGET_H
