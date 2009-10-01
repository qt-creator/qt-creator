#include "detailsbutton.h"

using namespace Utils;

DetailsButton::DetailsButton(QWidget *parent)
#ifdef Q_OS_MAC
    : QPushButton(parent),
#else
    : QToolButton(parent),
#endif
    m_checked(false)
{
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacSmallSize);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
#else
    setCheckable(true);
#endif
    setText(tr("Show Details"));
    connect(this, SIGNAL(clicked()),
            this, SLOT(onClicked()));
}

void DetailsButton::onClicked()
{
    m_checked = !m_checked;
}

bool DetailsButton::isToggled()
{
    return m_checked;
}
