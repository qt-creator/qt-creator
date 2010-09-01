#ifndef DEBUGGERLANGUAGECHOOSER_H
#define DEBUGGERLANGUAGECHOOSER_H

#include <QWidget>
#include "utils_global.h"

QT_FORWARD_DECLARE_CLASS(QCheckBox);

namespace Utils {

class QTCREATOR_UTILS_EXPORT DebuggerLanguageChooser : public QWidget
{
    Q_OBJECT
public:
    explicit DebuggerLanguageChooser(QWidget *parent = 0);

    void setCppChecked(bool value);
    void setQmlChecked(bool value);

signals:
    void cppLanguageToggled(bool value);
    void qmlLanguageToggled(bool value);

private slots:
    void useCppDebuggerToggled(bool toggled);
    void useQmlDebuggerToggled(bool toggled);

private:
    QCheckBox *m_useCppDebugger;
    QCheckBox *m_useQmlDebugger;
};

} // namespace Utils

#endif // DEBUGGERLANGUAGECHOOSER_H
