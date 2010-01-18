#ifndef TARGETSETTINGSWIDGET_H
#define TARGETSETTINGSWIDGET_H

#include "targetselector.h"

#include <QWidget>

namespace ProjectExplorer {
namespace Internal {

namespace Ui {
    class TargetSettingsWidget;
}

class TargetSettingsWidget : public QWidget {
    Q_OBJECT
public:
    explicit TargetSettingsWidget(QWidget *parent = 0);
    ~TargetSettingsWidget();

    void setCentralWidget(QWidget *widget);

    QString targetNameAt(int index) const;
    int targetCount() const;
    int currentIndex() const;
    int currentSubIndex() const;

public slots:
    void addTarget(const QString &name);
    void removeTarget(int index);

signals:
    void addButtonClicked();
    void currentIndexChanged(int targetIndex, int subIndex);

protected:
    void changeEvent(QEvent *e);

private:
    void updateTargetSelector();
    Ui::TargetSettingsWidget *ui;

    TargetSelector *m_targetSelector;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSETTINGSWIDGET_H
