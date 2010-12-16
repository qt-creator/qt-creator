#ifndef DEBUGGERLANGUAGECHOOSER_H
#define DEBUGGERLANGUAGECHOOSER_H

#include "utils_global.h"

#include <QtGui/QWidget>

QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSpinBox)

namespace Utils {

class QTCREATOR_UTILS_EXPORT DebuggerLanguageChooser : public QWidget
{
    Q_OBJECT
public:
    explicit DebuggerLanguageChooser(QWidget *parent = 0);

    bool cppChecked() const;
    bool qmlChecked() const;
    uint qmlDebugServerPort() const;

    void setCppChecked(bool value);
    void setQmlChecked(bool value);
    void setQmlDebugServerPort(uint port);

signals:
    void cppLanguageToggled(bool value);
    void qmlLanguageToggled(bool value);
    void qmlDebugServerPortChanged(uint port);

private slots:
    void useCppDebuggerToggled(bool toggled);
    void useQmlDebuggerToggled(bool toggled);
    void onDebugServerPortChanged(int port);

private:
    QCheckBox *m_useCppDebugger;
    QCheckBox *m_useQmlDebugger;
    QSpinBox *m_debugServerPort;
    QLabel *m_debugServerPortLabel;
};

} // namespace Utils

#endif // DEBUGGERLANGUAGECHOOSER_H
