#ifndef STYLEDOUTPUTPANEPLACEHOLDER_H
#define STYLEDOUTPUTPANEPLACEHOLDER_H

#include <coreplugin/outputpane.h>

QT_FORWARD_DECLARE_CLASS(QChildEvent)

class StyledOutputpanePlaceHolder : public Core::OutputPanePlaceHolder
{
public:
    StyledOutputpanePlaceHolder(Core::IMode *mode, QSplitter *parent = 0);

protected:
    void childEvent(QChildEvent *event);
private:
    QString m_customStylesheet;

};

#endif // STYLEDOUTPUTPANEPLACEHOLDER_H
