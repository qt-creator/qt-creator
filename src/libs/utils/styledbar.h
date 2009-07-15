#ifndef STYLEDBAR_H
#define STYLEDBAR_H

#include "utils_global.h"

#include <QtGui/QWidget>

namespace Core {
namespace Utils {

class QTCREATOR_UTILS_EXPORT StyledBar : public QWidget
{
public:
    StyledBar(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);
};

} // Utils
} // Core

#endif // STYLEDBAR_H
