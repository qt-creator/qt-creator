#ifndef DETAILSBUTTON_H
#define DETAILSBUTTON_H

#include <QtGui/QPushButton>
#include <QtGui/QToolButton>

#include "utils_global.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT DetailsButton
#ifdef Q_OS_MAC
    : public QPushButton
#else
    : public QToolButton
#endif
{
    Q_OBJECT
public:
    DetailsButton(QWidget *parent=0);
public slots:
    void onClicked();
private:
    bool m_checked;
};
}
#endif // DETAILSBUTTON_H
