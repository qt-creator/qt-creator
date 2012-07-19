/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QmlPropertyView_h
#define QmlPropertyView_h

#include <qmlmodelview.h>
#include <declarativewidgetview.h>
#include <QHash>
#include <QDeclarativePropertyMap>
#include <QStackedWidget>
#include <QTimer>

#include "qmlanchorbindingproxy.h"
#include "designerpropertymap.h"
#include "propertyeditorvalue.h"
#include "propertyeditorcontextobject.h"

QT_BEGIN_NAMESPACE
class QShortcut;
class QStackedWidget;
class QTimer;
QT_END_NAMESPACE

class PropertyEditorValue;

namespace QmlDesigner {

class PropertyEditorTransaction;
class CollapseButton;
class StackedWidget;

class PropertyEditor: public QmlModelView
{
    Q_OBJECT

    class NodeType {
    public:
        NodeType(PropertyEditor *propertyEditor);
        ~NodeType();

        void setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor);
        void initialSetup(const QString &typeName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor);
        void setValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value);

        DeclarativeWidgetView *m_view;
        Internal::QmlAnchorBindingProxy m_backendAnchorBinding;
        DesignerPropertyMap<PropertyEditorValue> m_backendValuesPropertyMap;
        QScopedPointer<PropertyEditorTransaction> m_propertyEditorTransaction;
        QScopedPointer<PropertyEditorValue> m_dummyPropertyEditorValue;
        QScopedPointer<PropertyEditorContextObject> m_contextObject;
    };

public:
    PropertyEditor(QWidget *parent = 0);
    ~PropertyEditor();

    void setQmlDir(const QString &qmlDirPath);

    QWidget *widget();

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);

    void propertiesAdded(const NodeState &state, const QList<NodeProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void propertyValuesChanged(const NodeState &state, const QList<NodeProperty>& propertyList);

    void modelAttached(Model *model);

    void modelAboutToBeDetached(Model *model);

    ModelState modelState() const;

    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);

    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);

    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);

    void resetView();
    void actualStateChanged(const ModelNode &node);

protected:
    void timerEvent(QTimerEvent *event);
    void otherPropertyChanged(const QmlObjectNode &, const QString &propertyName);
    void transformChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);

    void setupPane(const QString &typeName);
    void setValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value);

private slots:
    void reloadQml();
    void changeValue(const QString &name);
    void changeExpression(const QString &name);
    void updateSize();
    void setupPanes();

private: //functions
    QString qmlFileName(const NodeMetaInfo &nodeInfo) const;
    QUrl fileToUrl(const QString &filePath) const;
    QUrl qmlForNode(const ModelNode &modelNode, QString &className) const;
    QString locateQmlFile(const QString &relativePath) const;
    void select(const ModelNode& node);

    void delayedResetView();


private: //variables
    ModelNode m_selectedNode;
    QWidget *m_parent;
    QShortcut *m_updateShortcut;
    int m_timerId;
    StackedWidget* m_stackedWidget;
    QString m_qmlDir;
    QHash<QString, NodeType *> m_typeHash;
    NodeType *m_currentType;
    bool m_locked;
    bool m_setupCompleted;
    QTimer *m_singleShotTimer;
};


class StackedWidget : public QStackedWidget
{
Q_OBJECT

public:
    StackedWidget(QWidget *parent = 0) : QStackedWidget(parent) {}

signals:
    void resized();
protected:
    void resizeEvent(QResizeEvent * event)
    {
        QStackedWidget::resizeEvent(event);
        emit resized();
    }
};
}

#endif // QmlPropertyView_h
