#include "detailsbutton.h"

using namespace Utils;

DetailsButton::DetailsButton(QWidget *parent)
#ifdef Q_OS_MAC
    : QPushButton(parent)
#else
    : QToolButton(parent)
#endif
{
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacSmallSize);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
#else
    setCheckable(true);
#endif
    setText(tr("Details"));
}
