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

    bool isAddButtonEnabled() const;
    bool isRemoveButtonEnabled() const;

public slots:
    void addTarget(const QString &name);
    void insertTarget(int index, const QString &name);
    void markActive(int index);
    void removeTarget(int index);
    void setCurrentIndex(int index);
    void setCurrentSubIndex(int index);
    void setAddButtonEnabled(bool enabled);
    void setRemoveButtonEnabled(bool enabled);

signals:
    void addButtonClicked();
    void removeButtonClicked();
    void currentChanged(int targetIndex, int subIndex);

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
