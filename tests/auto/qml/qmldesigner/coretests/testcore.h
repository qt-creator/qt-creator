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

#ifndef TESTCORE_H
#define TESTCORE_H

#include <QObject>

#include <QtTest>


class TestCore : public QObject
{
    Q_OBJECT
public:
    TestCore();

private slots:
    void initTestCase();
    void cleanupTestCase();

    //
    // unit tests Model, ModelNode, NodeProperty, AbstractView
    //
    void testModelViewNotification();
    void testModelCreateSubNode();
    void testModelCreateCoreModel();
    void testModelCreateInvalidSubNode();
    void testModelCreateRect();
    void testModelRemoveNode();
    void testModelRootNode();
    void testModelReorderSiblings();
    void testModelAddAndRemoveProperty();
    void testModelSliding();
    void testModelBindings();
    void testDynamicProperties();
    void testModelDefaultProperties();
    void testModelBasicOperations();
    void testModelResolveIds();
    void testModelNodeListProperty();
    void testModelPropertyValueTypes();
    void testModelNodeInHierarchy();

    //
    // unit tests MetaInfo, NodeMetaInfo, PropertyMetaInfo
    //
    void testMetaInfo();
    void testMetaInfoDotProperties();
    void testMetaInfoListProperties();

    //
    // unit tests Rewriter
    //
    void testRewriterView();
    void testRewriterErrors();
    void testRewriterChangeId();
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

    //
    // integration tests
    //
    void testBasicStates();
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

    //
    // regression tests
    //

    void reparentingNodeLikeDragAndDrop();

    //
    // old tests
    //

    void loadEmptyCoreModel();
    void saveEmptyCoreModel();
    void loadAttributesInCoreModel();
    void saveAttributesInCoreModel();
    void loadComponentPropertiesInCoreModel();
    void loadSubItems();

    void attributeChangeSynchronizer();
    void attributeAdditionSynchronizer();
    void attributeRemovalSynchronizer();
    void childAddedSynchronizer();
    void childRemovedSynchronizer();
    void childReplacedSynchronizer();

    void createInvalidCoreModel();

    void loadQml();
    void subItemMetaInfo();




    void defaultPropertyValues();

    // Detailed tests for the rewriting:

    // Anchor tests:
    void loadAnchors();
    void changeAnchors();
    void anchorToSibling();
    void anchorsAndStates();
    void removeFillAnchorByDetaching();
    void removeFillAnchorByChanging();
    void removeCenteredInAnchorByDetaching();

    void sideIndex();

    // Property bindings:
    void loadPropertyBinding();
    void changePropertyBinding();

    void loadObjectPropertyBinding();

    // Bigger tests:
    void loadWelcomeScreen();

    // Object bindings as properties:
    void loadGradient();
    void changeGradientId();

    // Copy & Paste:
    void copyAndPasteInSingleModel();
    void copyAndPasteBetweenModels();

    // Editing inline Components
    void changeSubModel();
    void changeInlineComponent();

    // Testing imports
    void changeImports();

    void testIfChangePropertyIsRemoved();
    void testAnchorsAndStates();

    // Testing states
    void testStatesWithAnonymousTargets();
};

#endif // TESTCORE_H
