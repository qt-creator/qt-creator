#ifndef LOGGERMODE_H
#define LOGGERMODE_H

#include <coreplugin/imode.h>
#include <projectexplorer/project.h>
class QWidget;

struct LoggerModeData;
class LoggerMode :public Core::IMode
{
    Q_OBJECT

public:
    LoggerMode();
    ~LoggerMode();

    // IMode
    QString name() const;
    QIcon icon() const;
    int priority() const;
    QWidget *widget();
    const char *uniqueModeName() const;
    QList<int> context() const;
    void activated();
    QString contextHelpId() const { return QLatin1String("Qt Creator"); }

protected slots:
    void addNewStackWidgetPage(const QString projectName);
    void addItem();

private:
    LoggerModeData *d;
};

#endif // NEWMODE_H
