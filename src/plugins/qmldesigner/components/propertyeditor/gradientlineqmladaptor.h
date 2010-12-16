#ifndef GRADIENTLINEQMLADAPTOR_H
#define GRADIENTLINEQMLADAPTOR_H

#include <qmleditorwidgets/gradientline.h>
#include <propertyeditorvalue.h>
#include <qmlitemnode.h>

namespace QmlDesigner {

class GradientLineQmlAdaptor : public QmlEditorWidgets::GradientLine
{
    Q_OBJECT

    Q_PROPERTY(QVariant itemNode READ itemNode WRITE setItemNode NOTIFY itemNodeChanged)

public:
    explicit GradientLineQmlAdaptor(QWidget *parent = 0);

    static void registerDeclarativeType();

signals:
    void itemNodeChanged();

public slots:
    void setupGradient();
    void writeGradient();
    void deleteGradient();

private:
    QVariant itemNode() const { return QVariant::fromValue(m_itemNode.modelNode()); }
    void setItemNode(const QVariant &itemNode);

    QmlItemNode m_itemNode;

};

} //QmlDesigner

#endif // GRADIENTLINEQMLADAPTOR_H
