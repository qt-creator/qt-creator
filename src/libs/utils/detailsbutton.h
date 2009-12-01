#ifndef DETAILSBUTTON_H
#define DETAILSBUTTON_H

#include <QtGui/QAbstractButton>
#include <QtGui/QPixmap>

#include "utils_global.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT DetailsButton : public QAbstractButton
{
    Q_OBJECT
public:
    DetailsButton(QWidget *parent = 0);

    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *e);

private:
    QPixmap cacheRendering(const QSize &size, bool checked);
    QPixmap m_checkedPixmap;
    QPixmap m_uncheckedPixmap;
};
}
#endif // DETAILSBUTTON_H
