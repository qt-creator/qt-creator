// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"
#include "import.h"
#include "nodelistproperty.h"
#include "modelnode.h"

#include <qmljs/qmljsscopechain.h>

#include <QStringList>
#include <QTimer>

namespace QmlDesigner {

class RewriterView;
class DocumentMessage;

struct QmlTypeData;

namespace Internal {

class DifferenceHandler;
class ReadingContext;

class TextToModelMerger
{
    TextToModelMerger(const TextToModelMerger&);
    TextToModelMerger &operator=(const TextToModelMerger&);

public:
    static QmlJS::Document::MutablePtr createParsedDocument(const QUrl &url, const QString &data, QList<DocumentMessage> *errors);

    TextToModelMerger(RewriterView *reWriterView);
    bool isActive() const;

    void setupImports(const QmlJS::Document::Ptr &doc, DifferenceHandler &differenceHandler);
    void setupPossibleImports();
    void setupUsedImports();
    bool load(const QString &data, DifferenceHandler &differenceHandler);

    RewriterView *view() const
    { return m_rewriterView; }

    const QmlJS::ScopeChain *scopeChain() const
    { return m_scopeChain.data(); }

    const QmlJS::Document *document() const
    { return m_document.data(); }

    const QmlJS::ViewerContext &vContext() const
    { return m_vContext; }

protected:
    void setActive(bool active);

public:
    void syncNode(ModelNode &modelNode,
                  QmlJS::AST::UiObjectMember *astNode,
                  ReadingContext *context,
                  DifferenceHandler &differenceHandler);
    QmlDesigner::PropertyName syncScriptBinding(ModelNode &modelNode,
                              const QString &prefix,
                              QmlJS::AST::UiScriptBinding *script,
                              ReadingContext *context,
                              DifferenceHandler &differenceHandler);
    void syncNodeId(ModelNode &modelNode, const QString &astObjectId,
                    DifferenceHandler &differenceHandler);
    void syncNodeProperty(AbstractProperty &modelProperty,
                          QmlJS::AST::UiObjectBinding *binding,
                          ReadingContext *context,
                          const TypeName &astType,
                          DifferenceHandler &differenceHandler);
    void syncExpressionProperty(AbstractProperty &modelProperty,
                                const QString &javascript,
                                const TypeName &astType,
                                DifferenceHandler &differenceHandler);
    void syncSignalHandler(AbstractProperty &modelProperty,
                           const QString &javascript,
                           DifferenceHandler &differenceHandler);
    void syncArrayProperty(AbstractProperty &modelProperty,
                           const QList<QmlJS::AST::UiObjectMember *> &arrayMembers,
                           ReadingContext *context,
                           DifferenceHandler &differenceHandler);
    void syncSignalDeclarationProperty(AbstractProperty &modelProperty,
                            const QString &signature,
                            DifferenceHandler &differenceHandler);
    void syncVariantProperty(AbstractProperty &modelProperty,
                             const QVariant &astValue,
                             const TypeName &astType,
                             DifferenceHandler &differenceHandler);
    void syncNodeListProperty(NodeListProperty &modelListProperty,
                              const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                              ReadingContext *context,
                              DifferenceHandler &differenceHandler);
    ModelNode createModelNode(const QmlDesigner::NodeMetaInfo &nodeMetaInfo,
                              const TypeName &typeName,
                              int majorVersion,
                              int minorVersion,
                              bool isImplicitComponent,
                              QmlJS::AST::UiObjectMember *astNode,
                              ReadingContext *context,
                              DifferenceHandler &differenceHandler);
    QStringList syncGroupedProperties(ModelNode &modelNode,
                                      const QString &name,
                                      QmlJS::AST::UiObjectMemberList *members,
                                      ReadingContext *context,
                                      DifferenceHandler &differenceHandler);

    void setupComponentDelayed(const ModelNode &node, bool synchronous);
    void setupCustomParserNodeDelayed(const ModelNode &node, bool synchronous);
    void clearImplicitComponentDelayed(const ModelNode &node, bool synchronous);

    void delayedSetup();

    QSet<QPair<QString, QString>> qrcMapping() const;

    QList<QmlTypeData> getQMLSingletons() const;

    void clearPossibleImportKeys();

private:
    void setupCustomParserNode(const ModelNode &node);
    void setupComponent(const ModelNode &node);
    void clearImplicitComponent(const ModelNode &node);
    void collectLinkErrors(QList<DocumentMessage> *errors, const ReadingContext &ctxt);
    void collectImportErrors(QList<DocumentMessage> *errors);
    void collectSemanticErrorsAndWarnings(QList<DocumentMessage> *errors,
                                          QList<DocumentMessage> *warnings);
    void populateQrcMapping(const QString &filePath);
    void addIsoIconQrcMapping(const QUrl &fileUrl);

    static QString textAt(const QmlJS::Document::Ptr &doc,
                          const QmlJS::SourceLocation &location);
    static QString textAt(const QmlJS::Document::Ptr &doc,
                          const QmlJS::SourceLocation &from,
                          const QmlJS::SourceLocation &to);

private:
    RewriterView *m_rewriterView;
    bool m_isActive;
    QSharedPointer<const QmlJS::ScopeChain> m_scopeChain;
    QmlJS::Document::Ptr m_document;
    QTimer m_setupTimer;
    QSet<ModelNode> m_setupComponentList;
    QSet<ModelNode> m_setupCustomParserList;
    QSet<ModelNode> m_clearImplicitComponentList;
    QmlJS::ViewerContext m_vContext;
    QSet<QPair<QString, QString> > m_qrcMapping;
    Imports m_possibleModules;
    int m_previousPossibleModulesSize = -1;
    bool m_hasVersionlessImport = false;
};

class DifferenceHandler
{
public:
    DifferenceHandler(TextToModelMerger *textToModelMerger):
            m_merger(textToModelMerger)
    {}
    virtual ~DifferenceHandler() = default;

    virtual void modelMissesImport(const QmlDesigner::Import &import) = 0;
    virtual void importAbsentInQMl(const QmlDesigner::Import &import) = 0;
    virtual void bindingExpressionsDiffer(BindingProperty &modelProperty,
                                          const QString &javascript,
                                          const TypeName &astType) = 0;
    virtual void signalHandlerSourceDiffer(SignalHandlerProperty &modelProperty,
                                          const QString &javascript) = 0;
    virtual void signalDeclarationSignatureDiffer(SignalDeclarationProperty &modelProperty,
                                                       const QString &signature) = 0;
    virtual void shouldBeBindingProperty(AbstractProperty &modelProperty,
                                         const QString &javascript,
                                         const TypeName &astType) = 0;
    virtual void shouldBeSignalHandlerProperty(AbstractProperty &modelProperty,
                                         const QString &javascript) = 0;
    virtual void shouldBeSignalDeclarationProperty(AbstractProperty &modelProperty,
                                               const QString &signature) = 0;
    virtual void shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                          const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                                          ReadingContext *context) = 0;
    virtual void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicTypeName) = 0;
    virtual void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicTypeName) = 0;
    virtual void shouldBeNodeProperty(AbstractProperty &modelProperty,
                                      const NodeMetaInfo &nodeMetaInfo,
                                      const TypeName &typeName,
                                      int majorVersion,
                                      int minorVersion,
                                      QmlJS::AST::UiObjectMember *astNode,
                                      const TypeName &dynamicPropertyType,
                                      ReadingContext *context)
        = 0;
    virtual void modelNodeAbsentFromQml(ModelNode &modelNode) = 0;
    virtual ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                                   ReadingContext *context,
                                                   QmlJS::AST::UiObjectMember *arrayMember) = 0;
    virtual void typeDiffers(bool isRootNode,
                             ModelNode &modelNode,
                             const NodeMetaInfo &nodeMetaInfo,
                             const TypeName &typeName,
                             int majorVersion,
                             int minorVersion,
                             QmlJS::AST::UiObjectMember *astNode,
                             ReadingContext *context)
        = 0;
    virtual void propertyAbsentFromQml(AbstractProperty &modelProperty) = 0;
    virtual void idsDiffer(ModelNode &modelNode, const QString &qmlId) = 0;
    virtual bool isAmender() const = 0;

protected:
    TextToModelMerger *m_merger;
};

class ModelValidator: public DifferenceHandler
{
public:
    ModelValidator(TextToModelMerger *textToModelMerger):
            DifferenceHandler(textToModelMerger)
    {}
    ~ModelValidator() override = default;

    void modelMissesImport(const QmlDesigner::Import &import) override;
    void importAbsentInQMl(const QmlDesigner::Import &import) override;
    void bindingExpressionsDiffer(BindingProperty &modelProperty,
                                  const QString &javascript,
                                  const TypeName &astType) override;
    void shouldBeBindingProperty(AbstractProperty &modelProperty,
                                 const QString &javascript,
                                 const TypeName &astType) override;
    void signalHandlerSourceDiffer(SignalHandlerProperty &modelProperty,
                                   const QString &javascript) override;
    void signalDeclarationSignatureDiffer(SignalDeclarationProperty &modelProperty,
                                          const QString &signature) override;
    void shouldBeSignalDeclarationProperty(AbstractProperty &modelProperty,
                                           const QString &signature) override;
    void shouldBeSignalHandlerProperty(AbstractProperty &modelProperty,
                                       const QString &javascript) override;
    void shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                  const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                                  ReadingContext *context) override;
    void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicTypeName) override;
    void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicTypeName) override;
    void shouldBeNodeProperty(AbstractProperty &modelProperty,
                              const NodeMetaInfo &nodeMetaInfo,
                              const TypeName &typeName,
                              int majorVersion,
                              int minorVersion,
                              QmlJS::AST::UiObjectMember *astNode,
                              const TypeName &dynamicPropertyType,
                              ReadingContext *context) override;

    void modelNodeAbsentFromQml(ModelNode &modelNode) override;
    ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                           ReadingContext *context,
                                           QmlJS::AST::UiObjectMember *arrayMember) override;
    void typeDiffers(bool isRootNode,
                     ModelNode &modelNode,
                     const NodeMetaInfo &nodeMetaInfo,
                     const TypeName &typeName,
                     int majorVersion,
                     int minorVersion,
                     QmlJS::AST::UiObjectMember *astNode,
                     ReadingContext *context) override;
    void propertyAbsentFromQml(AbstractProperty &modelProperty) override;
    void idsDiffer(ModelNode &modelNode, const QString &qmlId) override;
    bool isAmender() const override {return false; }
};

class ModelAmender: public DifferenceHandler
{
public:
    ModelAmender(TextToModelMerger *textToModelMerger):
            DifferenceHandler(textToModelMerger)
    {}
    ~ModelAmender() override = default;

    void modelMissesImport(const QmlDesigner::Import &import) override;
    void importAbsentInQMl(const QmlDesigner::Import &import) override;
    void bindingExpressionsDiffer(BindingProperty &modelProperty,
                                  const QString &javascript,
                                  const TypeName &astType) override;
    void shouldBeBindingProperty(AbstractProperty &modelProperty,
                                 const QString &javascript,
                                 const TypeName &astType) override;
    void signalHandlerSourceDiffer(SignalHandlerProperty &modelProperty,
                                   const QString &javascript) override;
    void signalDeclarationSignatureDiffer(SignalDeclarationProperty &modelProperty,
                                          const QString &signature) override;
    void shouldBeSignalDeclarationProperty(AbstractProperty &modelProperty,
                                           const QString &signature) override;
    void shouldBeSignalHandlerProperty(AbstractProperty &modelProperty,
                                       const QString &javascript) override;
    void shouldBeNodeListProperty(AbstractProperty &modelProperty,
                                  const QList<QmlJS::AST::UiObjectMember *> arrayMembers,
                                  ReadingContext *context) override;
    void variantValuesDiffer(VariantProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicType) override;
    void shouldBeVariantProperty(AbstractProperty &modelProperty, const QVariant &qmlVariantValue, const TypeName &dynamicTypeName) override;
    void shouldBeNodeProperty(AbstractProperty &modelProperty,
                              const NodeMetaInfo &nodeMetaInfo,
                              const TypeName &typeName,
                              int majorVersion,
                              int minorVersion,
                              QmlJS::AST::UiObjectMember *astNode,
                              const TypeName &dynamicPropertyType,
                              ReadingContext *context) override;

    void modelNodeAbsentFromQml(ModelNode &modelNode) override;
    ModelNode listPropertyMissingModelNode(NodeListProperty &modelProperty,
                                           ReadingContext *context,
                                           QmlJS::AST::UiObjectMember *arrayMember) override;
    void typeDiffers(bool isRootNode,
                     ModelNode &modelNode,
                     const NodeMetaInfo &nodeMetaInfo,
                     const TypeName &typeName,
                     int majorVersion,
                     int minorVersion,
                     QmlJS::AST::UiObjectMember *astNode,
                     ReadingContext *context) override;
    void propertyAbsentFromQml(AbstractProperty &modelProperty) override;
    void idsDiffer(ModelNode &modelNode, const QString &qmlId) override;
    bool isAmender() const override {return true; }
};

} //Internal
} //QmlDesigner
