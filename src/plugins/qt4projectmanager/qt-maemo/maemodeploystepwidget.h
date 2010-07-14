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
    Q_SLOT void handleModelsCreated();
    virtual void init();
    virtual QString summaryText() const;
    virtual QString displayName() const;

    Q_SLOT void handleDeviceUpdate();

    Ui::MaemoDeployStepWidget *ui;
    MaemoDeployStep * const m_step;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMODEPLOYSTEPWIDGET_H
