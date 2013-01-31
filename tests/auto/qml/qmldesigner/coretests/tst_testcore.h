/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TESTCORE_H
#define TESTCORE_H

#include <QObject>

#include <QtTest>


class tst_TestCore : public QObject
{
    Q_OBJECT
public:
    tst_TestCore();

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
    void testMetaInfoQtQuick1Vs2();
    void testMetaInfoListProperties();
    void testQtQuick20Basic();
    void testQtQuick20BasicRectangle();

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
    void testModelPropertyValueTypes();
    void testModelNodeInHierarchy();
    void testModelNodeIsAncestorOf();

    //
    // unit tests Rewriter
    //
    void testRewriterView();
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
    void testInstancesStates();
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
    void testSubComponentManager();
    void testAnchorsAndRewriting();
    void testAnchorsAndRewritingCenter();

    //
    // regression tests
    //

    void reparentingNodeLikeDragAndDrop();
    void testRewriterForGradientMagic();

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
};

#endif // TESTCORE_H
