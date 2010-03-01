/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef TEXTTOMODELMERGER_H
#define TEXTTOMODELMERGER_H

#include "corelib_global.h"
#include "import.h"
#include "nodelistproperty.h"
#include "modelnode.h"

#include <private/qdeclarativedom_p.h>

namespace QmlDesigner {

class CORESHARED_EXPORT RewriterView;

namespace Internal {

class DifferenceHandler;

class TextToModelMerger
{
    TextToModelMerger(const TextToModelMerger&);
    TextToModelMerger &operator=(const TextToModelMerger&);

public:
    TextToModelMerger(RewriterView *reWriterView);
    bool isActive() const;

    void setupImports(QDeclarativeDomDocument &doc, DifferenceHandler &differenceHandler);
    bool load(const QByteArray &data, DifferenceHandler &differenceHandler);

    RewriterView *view() const
    { return m_rewriterView; }

protected:
    void setActive(bool active);

public:
    void syncNode(ModelNode &modelNode,
                  const QDeclarativeDomObject &qmlObject,
                  DifferenceHandler &differenceHandler);
    void syncProperty(AbstractProperty &modelProperty,
                      const QDeclarativeDomProperty &qmlProperty,
                      const QDeclarativeDomDynamicProperty &qmlDynamicProperty,
                      DifferenceHandler &differenceHandler);
    void syncNodeListProperty(NodeListProperty &modelListProperty,
                              const QDeclarativeDomList &domList,
                              DifferenceHandler &differenceHandler);
    ModelNode createModelNode(const QDeclarativeDomObject &domObject,
                              DifferenceHandler &differenceHandler);
    static QVariant convertToVariant(const ModelNode &node,
                                     const QDeclarativeDomProperty &qmlProperty,
                                     const QDeclarativeDomDynamicProperty &qmlDynamicProperty);

private:
    static bool isSignalPropertyName(const QString &signalName);
    void setupComponent(const ModelNode &node);

private:
    RewriterView *m_rewriterView;
    bool m_isActive;
};

class DifferenceHandler
{
public:
    DifferenceHandler(TextToModelMerger *textToModelMerger):
            m_merger(textToModelMerger)
    {}
    virtual ~DifferenceHandler()
    {}

    virtual void modelMissesImport(const Import &import) = 0;
    virtual void importAbsentInQMl(const Import &import) = 0;
    virtual void bindingExpressionsDiffer(BindingProperty &modelProperty, const QString &QDeclarativeBinding) = 0;
    virtual void shouldBeBindingProperty(AbstractProperty &modelProperty, const QString &QDeclarativeBinding) = 0;
    virtual void shouldBeNodeListProperty(AbstractProperty &modelProperty, const QDeclarativeDomList &domList) = 0;
    virtual void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName) = 0;
    virtual void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName) = 0;
    virtual void shouldBeNodeProperty(AbstractProperty &modelProperty, const QDeclarativeDomObject &qmlObject) = 0;
    virtual void modelNodeAbsentFromQml(ModelNode &modelNode) = 0;
    virtual ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty, const QDeclarativeDomObject &qmlObject) = 0;
    virtual void typeDiffers(bool isRootNode, ModelNode &modelNode, const QDeclarativeDomObject &domObject) = 0;
    virtual void propertyAbsentFromQml(AbstractProperty &modelProperty) = 0;
    virtual void idsDiffer(ModelNode &modelNode, const QString &qmlId) = 0;

protected:
    TextToModelMerger *m_merger;
};

class ModelValidator: public DifferenceHandler
{
public:
    ModelValidator(TextToModelMerger *textToModelMerger):
            DifferenceHandler(textToModelMerger)
    {}
    ~ModelValidator()
    {}

    virtual void modelMissesImport(const Import &import);
    virtual void importAbsentInQMl(const Import &import);
    virtual void bindingExpressionsDiffer(BindingProperty &modelProperty, const QString &QDeclarativeBinding);
    virtual void shouldBeBindingProperty(AbstractProperty &modelProperty, const QString &QDeclarativeBinding);
    virtual void shouldBeNodeListProperty(AbstractProperty &modelProperty, const QDeclarativeDomList &domList);
    virtual void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName);
    virtual void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName);
    virtual void shouldBeNodeProperty(AbstractProperty &modelProperty, const QDeclarativeDomObject &qmlObject);
    virtual void modelNodeAbsentFromQml(ModelNode &modelNode);
    virtual ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty, const QDeclarativeDomObject &qmlObject);
    virtual void typeDiffers(bool isRootNode, ModelNode &modelNode, const QDeclarativeDomObject &domObject);
    virtual void propertyAbsentFromQml(AbstractProperty &modelProperty);
    virtual void idsDiffer(ModelNode &modelNode, const QString &qmlId);
};

class ModelAmender: public DifferenceHandler
{
public:
    ModelAmender(TextToModelMerger *textToModelMerger):
            DifferenceHandler(textToModelMerger)
    {}
    ~ModelAmender()
    {}

    virtual void modelMissesImport(const Import &import);
    virtual void importAbsentInQMl(const Import &import);
    virtual void bindingExpressionsDiffer(BindingProperty &modelProperty, const QString &QDeclarativeBinding);
    virtual void shouldBeBindingProperty(AbstractProperty &modelProperty, const QString &QDeclarativeBinding);
    virtual void shouldBeNodeListProperty(AbstractProperty &modelProperty, const QDeclarativeDomList &domList);
    virtual void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicType);
    virtual void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const QString &dynamicTypeName);
    virtual void shouldBeNodeProperty(AbstractProperty &modelProperty, const QDeclarativeDomObject &qmlObject);
    virtual void modelNodeAbsentFromQml(ModelNode &modelNode);
    virtual ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty, const QDeclarativeDomObject &qmlObject);
    virtual void typeDiffers(bool isRootNode, ModelNode &modelNode, const QDeclarativeDomObject &domObject);
    virtual void propertyAbsentFromQml(AbstractProperty &modelProperty);
    virtual void idsDiffer(ModelNode &modelNode, const QString &qmlId);
};

} //Internal
} //QmlDesigner

#endif // TEXTTOMODELMERGER_H
