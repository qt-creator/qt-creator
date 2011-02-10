#ifndef LINEEDITACTION_H
#define LINEEDITACTION_H

#include <QWidgetAction>

namespace QmlDesigner {

class LineEditAction : public QWidgetAction
{
    Q_OBJECT
public:
    explicit LineEditAction(const QString &placeHolderText, QObject *parent = 0);

protected:
    QWidget *createWidget(QWidget *parent);

signals:
    void textChanged(const QString &text);

private:
    QString m_placeHolderText;
};

} // namespace QmlDesigner

#endif // LINEEDITACTION_H
