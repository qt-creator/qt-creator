#ifndef FONTSIZESPINBOX_H
#define FONTSIZESPINBOX_H

#include <QAbstractSpinBox>

namespace QmlDesigner {

class FontSizeSpinBox : public QAbstractSpinBox
{
    Q_OBJECT

     Q_PROPERTY(bool isPixelSize READ isPixelSize WRITE setIsPixelSize NOTIFY formatChanged)
     Q_PROPERTY(bool isPointSize READ isPointSize WRITE setIsPointSize NOTIFY formatChanged)
     Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

public:
    explicit FontSizeSpinBox(QWidget *parent = 0);

     bool isPixelSize() { return !m_isPointSize; }
     bool isPointSize() { return m_isPointSize; }

     void stepBy(int steps);

     QValidator::State validate (QString &input, int &pos) const;
     int value() const { return m_value; }

signals:
     void formatChanged();
     void valueChanged(int);

public slots:
     void setIsPointSize(bool b)
     {
         if (isPointSize() == b)
             return;

         m_isPointSize = b;
         setText();
         emit formatChanged();
     }

     void setIsPixelSize(bool b)
     {
         if (isPixelSize() == b)
             return;

         m_isPointSize = !b;
         setText();
         emit formatChanged();
     }


     void clear();
     void setValue (int val);

 protected:
    StepEnabled stepEnabled() const;
private slots:
    void onEditingFinished();
    void setText();

private:
    bool m_isPointSize;
    int m_value;

};

} //QmlDesigner

#endif // FONTSIZESPINBOX_H
