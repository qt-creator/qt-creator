#ifndef MAEMODEPLOYSTEPWIDGET_H
#define MAEMODEPLOYSTEPWIDGET_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MaemoDeployStepWidget;
}
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
class MaemoDeployStep;

class MaemoDeployStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    MaemoDeployStepWidget(MaemoDeployStep *step);
    ~MaemoDeployStepWidget();

private:
    Q_SLOT void handleDeviceUpdate();
    Q_SLOT void handleModelsCreated();
    Q_SLOT void handleDeviceConfigModelChanged();
    Q_SLOT void setCurrentDeviceConfig(int index);

    virtual void init();
    virtual QString summaryText() const;
    virtual QString displayName() const;

    Ui::MaemoDeployStepWidget *ui;
    MaemoDeployStep * const m_step;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEPWIDGET_H
