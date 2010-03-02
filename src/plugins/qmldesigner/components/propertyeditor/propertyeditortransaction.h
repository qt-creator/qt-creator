#ifndef PROPERTYEDITORTRANSACTION_H
#define PROPERTYEDITORTRANSACTION_H

#include "propertyeditor.h"

namespace QmlDesigner {

class PropertyEditorTransaction : public QObject
{
    Q_OBJECT
public:
    PropertyEditorTransaction(QmlDesigner::PropertyEditor *propertyEditor);

public slots:
    void start();
    void end();
protected:
     void timerEvent(QTimerEvent *event);

private:
    QmlDesigner::PropertyEditor *m_propertyEditor;
    QmlDesigner::RewriterTransaction m_rewriterTransaction;
};

} //QmlDesigner

#endif // PROPERTYEDITORTRANSACTION_H
