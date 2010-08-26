#ifndef LOGGERMODEWIDGET_H
#define LOGGERMODEWIDGET_H

#include <QWidget>

struct LoggerModeWidgetData;
class LoggerModeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LoggerModeWidget(const QString projectName, QWidget* parent = 0);
    ~LoggerModeWidget();

public slots:
    //void setProjectName(QString name);

protected slots:
    bool saveToFile();
    void startTimeLog();   
    void endTimeLog();
    void updateTime();

private:
    LoggerModeWidgetData* d;
};

#endif // NEWMODEWIDGET_H
