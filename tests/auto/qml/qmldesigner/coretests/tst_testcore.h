// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include <QtTest>

#include <extensionsystem/pluginmanager.h>


class tst_TestCore : public QObject
{
    Q_OBJECT
public:
    tst_TestCore();
    ~tst_TestCore();
private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();



    //
    // unit tests MetaInfo, NodeMetaInfo, PropertyMetaInfo
    //
    void testMetaInfo();
    void testMetaInfoSimpleType();
    void testMetaInfoUncreatableType();
    void testMetaInfoExtendedType();
    void testMetaInfoInterface();
    void testMetaInfoCustomType();
    void testMetaInfoEnums();
    void testMetaInfoProperties();
    void testMetaInfoDotProperties();
    void testMetaInfoQtQuickVersion2();
    void testMetaInfoListProperties();
    void testQtQuick20Basic();
    void testQtQuick20BasicRectangle();
    void testQtQuickControls2();
    void testImplicitComponents();
    void testRevisionedProperties();

    //
    // unit tests Model, ModelNode, NodeProperty, AbstractView
    //
    void testModelCreateCoreModel();
    void testModelCreateRect();
    void testModelViewNotification();
    void testModelCreateSubNode();
    void testModelRemoveNode();
    void testModelRootNode();
    void testModelReorderSiblings();
    void testModelAddAndRemoveProperty();
    void testModelSliding();
    void testModelBindings();
    void testModelDynamicProperties();
    void testModelDefaultProperties();
    void testModelBasicOperations();
    void testModelResolveIds();
    void testModelNodeListProperty();
    void testModelNodePropertyDynamic();
    void testModelNodePropertyDynamicSource();
    void testModelPropertyValueTypes();
    void testModelNodeInHierarchy();
    void testModelNodeIsAncestorOf();
    void testModelChangeType();
    void testModelSignalDefinition();

    //
    // unit tests Rewriter
    //
    void testRewriterView();
    void testRewriterView2();
    void testRewriterErrors();
    void testRewriterChangeId();
    void testRewriterRemoveId();
    void testRewriterChangeValueProperty();
    void testRewriterRemoveValueProperty();
    void testRewriterSignalProperty();
    void testRewriterObjectTypeProperty();
    void testRewriterPropertyChanges();
    void testRewriterListModel();
    void testRewriterAddProperty();
    void testRewriterAddPropertyInNestedObject();
    void testRewriterAddObjectDefinition();
    void testRewriterAddStatesArray();
    void testRewriterRemoveStates();
    void testRewriterRemoveObjectDefinition();
    void testRewriterRemoveScriptBinding();
    void testRewriterNodeReparenting();
    void testRewriterNodeReparentingWithTransaction();
    void testRewriterMovingInOut();
    void testRewriterMovingInOutWithTransaction();
    void testRewriterComplexMovingInOut();
    void testRewriterAutomaticSemicolonAfterChangedProperty();
    void testRewriterTransaction();
    void testRewriterId();
    void testRewriterNodeReparentingTransaction1();
    void testRewriterNodeReparentingTransaction2();
    void testRewriterNodeReparentingTransaction3();
    void testRewriterNodeReparentingTransaction4();
    void testRewriterAddNodeTransaction();
    void testRewriterComponentId();
    void testRewriterPropertyDeclarations();
    void testRewriterPropertyAliases();
    void testRewriterPositionAndOffset();
    void testRewriterFirstDefinitionInside();
    void testRewriterComponentTextModifier();
    void testRewriterPreserveType();
    void testRewriterForArrayMagic();
    void testRewriterWithSignals();
    void testRewriterNodeSliding();
    void testRewriterExceptionHandling();
    void testRewriterDynamicProperties();
    void testRewriterGroupedProperties();
    void testRewriterPreserveOrder();
    void testRewriterActionCompression();
    void testRewriterImports();
    void testRewriterChangeImports();
    void testRewriterUnicodeChars();
    void testRewriterTransactionAddingAfterReparenting();
    void testRewriterReparentToNewNode();
    void testRewriterBehaivours();
    void testRewriterSignalDefinition();

    //
    // unit tests QmlModelNodeFacade/QmlModelState
    //

    void testQmlModelStates();
    void testQmlModelAddMultipleStates();
    void testQmlModelRemoveStates();
    void testQmlModelStatesInvalidForRemovedNodes();
    void testQmlModelStateWithName();

    //
    // unit tests Instances
    //
    void testInstancesAttachToExistingModel();
    void testStatesWasInstances();
    void testInstancesIdResolution();
    void testInstancesNotInScene();
    void testInstancesBindingsInStatesStress();
    void testInstancesPropertyChangeTargets();
    void testInstancesDeletePropertyChanges();
    void testInstancesChildrenLowLevel();
    void testInstancesResourcesLowLevel();
    void testInstancesFlickableLowLevel();
    void testInstancesReorderChildrenLowLevel();

    //
    // integration tests
    //
    void testBasicStates();
    void testBasicStatesQtQuick20();
    void testStates();
    void testStatesBaseState();
    void testStatesRewriter();
    void testGradientsRewriter();
    void testTypicalRewriterOperations();
    void testBasicOperationsWithView();
    void testQmlModelView();
    void reparentingNode();
    void reparentingNodeInModificationGroup();
    void testRewriterTransactionRewriter();
    void testCopyModelRewriter1();
    void testCopyModelRewriter2();
    void testMergeModelRewriter1_data();
    void testMergeModelRewriter1();
    void testSubComponentManager();
    void testAnchorsAndRewriting();
    void testAnchorsAndRewritingCenter();

    //
    // regression tests
    //

    void reparentingNodeLikeDragAndDrop();
    void testRewriterForGradientMagic();
    void testStatesVersionFailing();

    //
    // old tests
    //

    void loadEmptyCoreModel();
    void saveEmptyCoreModel();
    void loadAttributesInCoreModel();
    void saveAttributesInCoreModel();
    void loadSubItems();


    void createInvalidCoreModel();

    void loadQml();

    void defaultPropertyValues();

    // Detailed tests for the rewriting:

    // Anchor tests:
    void loadAnchors();
    void changeAnchors();
    void anchorToSibling();
    void removeFillAnchorByDetaching();
    void removeFillAnchorByChanging();
    void removeCenteredInAnchorByDetaching();

    // Property bindings:
    void changePropertyBinding();

    // Bigger tests:
    void loadTestFiles();

    // Object bindings as properties:
    void loadGradient();
    void changeGradientId();

    // QMLAnnotations
    void writeAnnotations();
    void readAnnotations();

private:
    class std::unique_ptr<class ExternalDependenciesFake> externalDependencies;
    ExtensionSystem::PluginManager pm; // FIXME remove
};
