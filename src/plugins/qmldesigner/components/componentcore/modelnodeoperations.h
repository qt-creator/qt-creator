// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "selectioncontext.h"
#include <qmldesignercomponents_global.h>

#include <utils/fileutils.h>

namespace QmlDesigner {

class AddFilesResult
{
public:
    enum Status { Succeeded, Failed, Cancelled, Delayed };
    static constexpr char directoryPropName[] = "directory";

    static AddFilesResult cancelled(const QString &directory = {})
    {
        return AddFilesResult{Cancelled, directory};
    }

    static AddFilesResult failed(const QString &directory = {})
    {
        return AddFilesResult{Failed, directory};
    }

    static AddFilesResult succeeded(const QString &directory = {})
    {
        return AddFilesResult{Succeeded, directory};
    }

    static AddFilesResult delayed(QObject *delayedResult)
    {
        return AddFilesResult{Delayed, {}, delayedResult};
    }

    Status status() const { return m_status; }
    QString directory() const { return m_directory; }
    bool haveDelayedResult() const { return m_delayedResult != nullptr; }
    QObject *delayedResult() const { return m_delayedResult; }

private:
    AddFilesResult(Status status, const QString &directory, QObject *delayedResult = nullptr)
        : m_status{status}
        , m_directory{directory}
        , m_delayedResult{delayedResult}
    {}

    Status m_status;
    QString m_directory;
    QObject *m_delayedResult = nullptr;
};

namespace ModelNodeOperations {

bool goIntoComponent(const ModelNode &modelNode);
void jumpToCode(const ModelNode &modelNode);

void select(const SelectionContext &selectionState);
void deSelect(const SelectionContext &selectionState);
void cut(const SelectionContext &selectionState);
void copy(const SelectionContext &selectionState);
void deleteSelection(const SelectionContext &selectionState);
void toFront(const SelectionContext &selectionState);
void toBack(const SelectionContext &selectionState);
void raise(const SelectionContext &selectionState);
void lower(const SelectionContext &selectionState);
void paste(const SelectionContext &selectionState);
void undo(const SelectionContext &selectionState);
void redo(const SelectionContext &selectionState);
void setVisible(const SelectionContext &selectionState);
void setFillWidth(const SelectionContext &selectionState);
void setFillHeight(const SelectionContext &selectionState);
void resetSize(const SelectionContext &selectionState);
void resetPosition(const SelectionContext &selectionState);
void goIntoComponentOperation(const SelectionContext &selectionState);
void jumpToCodeOperation(const SelectionContext &selectionState);
void setId(const SelectionContext &selectionState);
void resetZ(const SelectionContext &selectionState);
void reverse(const SelectionContext &selectionState);
void anchorsFill(const SelectionContext &selectionState);
void anchorsReset(const SelectionContext &selectionState);
void layoutRowPositioner(const SelectionContext &selectionState);
void layoutColumnPositioner(const SelectionContext &selectionState);
void layoutGridPositioner(const SelectionContext &selectionState);
void layoutFlowPositioner(const SelectionContext &selectionState);
void layoutRowLayout(const SelectionContext &selectionState);
void layoutColumnLayout(const SelectionContext &selectionState);
void layoutGridLayout(const SelectionContext &selectionState);
void goImplementation(const SelectionContext &selectionState);
void addNewSignalHandler(const SelectionContext &selectionState);
void editMaterial(const SelectionContext &selectionContext);
void addSignalHandlerOrGotoImplementation(const SelectionContext &selectionState, bool addAlwaysNewSlot);
void removeLayout(const SelectionContext &selectionContext);
void removePositioner(const SelectionContext &selectionContext);
void moveToComponent(const SelectionContext &selectionContext);
void add3DAssetToContentLibrary(const SelectionContext &selectionContext);
PropertyName getIndexPropertyName(const ModelNode &modelNode);
void addItemToStackedContainer(const SelectionContext &selectionContext);
void increaseIndexOfStackedContainer(const SelectionContext &selectionContext);
void decreaseIndexOfStackedContainer(const SelectionContext &selectionContext);
void addTabBarToStackedContainer(const SelectionContext &selectionContext);
QMLDESIGNERCOMPONENTS_EXPORT AddFilesResult addFilesToProject(const QStringList &fileNames,
                                                              const QString &defaultDir,
                                                              bool showDialog = true);
AddFilesResult addImageToProject(const QStringList &fileNames, const QString &directory, bool showDialog = true);
AddFilesResult addFontToProject(const QStringList &fileNames, const QString &directory, bool showDialog = true);
AddFilesResult addSoundToProject(const QStringList &fileNames, const QString &directory, bool showDialog = true);
AddFilesResult addShaderToProject(const QStringList &fileNames, const QString &directory, bool showDialog = true);
AddFilesResult addVideoToProject(const QStringList &fileNames, const QString &directory, bool showDialog = true);
void createFlowActionArea(const SelectionContext &selectionContext);
void addTransition(const SelectionContext &selectionState);
void addFlowEffect(const SelectionContext &selectionState, const TypeName &typeName);
void addCustomFlowEffect(const SelectionContext &selectionState);
void setFlowStartItem(const SelectionContext &selectionContext);
void addToGroupItem(const SelectionContext &selectionContext);
void selectFlowEffect(const SelectionContext &selectionContext);
void mergeWithTemplate(const SelectionContext &selectionContext, ExternalDependenciesInterface &externalDependencies);
void removeGroup(const SelectionContext &selectionContext);
void editAnnotation(const SelectionContext &selectionContext);
void addMouseAreaFill(const SelectionContext &selectionContext);

void createView3D(const SelectionContext &selectionContext);
void createImage(const SelectionContext &selectionContext);
void createItem(const SelectionContext &selectionContext);
void createFlickable(const SelectionContext &selectionContext);
void createMouseArea(const SelectionContext &selectionContext);
void createDropArea(const SelectionContext &selectionContext);
void createFocusScope(const SelectionContext &selectionContext);
void createRectangle(const SelectionContext &selectionContext);
void createProgressBar(const SelectionContext &selectionContext);
void createText(const SelectionContext &selectionContext);
void createBusyIndicator(const SelectionContext &selectionContext);
void createTextArea(const SelectionContext &selectionContext);
void createFrame(const SelectionContext &selectionContext);
void createPage(const SelectionContext &selectionContext);
void createPane(const SelectionContext &selectionContext);
void createColorAnimation(const SelectionContext &selectionContext);
void createFrameAnimation(const SelectionContext &selectionContext);
void createNumberAnimation(const SelectionContext &selectionContext);
void createParallelAnimation(const SelectionContext &selectionContext);
void createPauseAnimation(const SelectionContext &selectionContext);
void createPropertyAction(const SelectionContext &selectionContext);
void createPropertyAnimation(const SelectionContext &selectionContext);
void createScriptAction(const SelectionContext &selectionContext);
void createSequentialAnimation(const SelectionContext &selectionContext);
void createTimer(const SelectionContext &selectionContext);
void createControl(const SelectionContext &selectionContext);
void createTabBar(const SelectionContext &selectionContext);
void createQQCToolBar(const SelectionContext &selectionContext);
void createTextEdit(const SelectionContext &selectionContext);
void createTextInput(const SelectionContext &selectionContext);
void createButton(const SelectionContext &selectionContext);
void createToolButton(const SelectionContext &selectionContext);
void createDelayButton(const SelectionContext &selectionContext);
void createTabButton(const SelectionContext &selectionContext);
void createCheckBox(const SelectionContext &selectionContext);
void createDial(const SelectionContext &selectionContext);
void createRadioButton(const SelectionContext &selectionContext);
void createRoundButton(const SelectionContext &selectionContext);
void createLabel(const SelectionContext &selectionContext);
void createSlider(const SelectionContext &selectionContext);
void createTumbler(const SelectionContext &selectionContext);
void createRangeSlider(const SelectionContext &selectionContext);
void createSpinBox(const SelectionContext &selectionContext);
void createSwitch(const SelectionContext &selectionContext);
void createTextField(const SelectionContext &selectionContext);
void createGridView(const SelectionContext &selectionContext);
void createAnimatedImage(const SelectionContext &selectionContext);
void createBorderImage(const SelectionContext &selectionContext);
void createAnimatedSprite(const SelectionContext &selectionContext);
void createListView(const SelectionContext &selectionContext);
void createSwipeView(const SelectionContext &selectionContext);
void createStackView(const SelectionContext &selectionContext);
void createPathView(const SelectionContext &selectionContext);
void createScrollView(const SelectionContext &selectionContext);
void createColumn(const SelectionContext &selectionContext);
void createFlow(const SelectionContext &selectionContext);
void createGrid(const SelectionContext &selectionContext);
void createRow(const SelectionContext &selectionContext);
void createNode(const SelectionContext &selectionContext);
void createOrthographicCamera(const SelectionContext &selectionContext);
void createPerspectiveCamera(const SelectionContext &selectionContext);
void createDirectionalLight(const SelectionContext &selectionContext);
void createPointLight(const SelectionContext &selectionContext);
void createSpotLight(const SelectionContext &selectionContext);
void createReflectionProbe(const SelectionContext &selectionContext);
void createCone(const SelectionContext &selectionContext);
void createCube(const SelectionContext &selectionContext);
void createCylinder(const SelectionContext &selectionContext);
void createModel(const SelectionContext &selectionContext);
void createPlane(const SelectionContext &selectionContext);
void createSphere(const SelectionContext &selectionContext);
void createComponent(const SelectionContext &selectionContext);
void createLoader(const SelectionContext &selectionContext);
void createRepeater(const SelectionContext &selectionContext);
void createCheckDelegate(const SelectionContext &selectionContext);
void createItemDelegate(const SelectionContext &selectionContext);
void createRadioDelegate(const SelectionContext &selectionContext);
void createSwipeDelegate(const SelectionContext &selectionContext);
void createSwitchDelegate(const SelectionContext &selectionContext);
void createEmbedded2dText(const SelectionContext &selectionContext);

void openSignalDialog(const SelectionContext &selectionContext);
void updateImported3DAsset(const SelectionContext &selectionContext);
void editIn3dView(const SelectionContext &selectionContext);
QMLDESIGNERCOMPONENTS_EXPORT Utils::FilePath findEffectFile(const ModelNode &effectNode);
void editInEffectComposer(const SelectionContext &selectionContext);

QMLDESIGNERCOMPONENTS_EXPORT Utils::FilePath getEffectsImportDirectory();
QMLDESIGNERCOMPONENTS_EXPORT QString getEffectsDefaultDirectory(const QString &defaultDir = {});
void openEffectComposer(const QString &filePath);
void openOldEffectMaker(const QString &filePath);
QString getEffectIcon(const QString &effectPath);
bool useLayerEffect();
bool validateEffect(const QString &effectPath);
bool isEffectComposerActivated();

Utils::FilePath getImagesDefaultDirectory();

//Item Library and Assets related drop operations
QMLDESIGNERCOMPONENTS_EXPORT ModelNode handleItemLibraryEffectDrop(const QString &effectPath,
                                                                   const ModelNode &targetNode);
void handleTextureDrop(const QMimeData *mimeData, const ModelNode &targetModelNode);
void handleMaterialDrop(const QMimeData *mimeData, const ModelNode &targetNode);
ModelNode handleItemLibraryImageDrop(const QString &imagePath,
                                     NodeAbstractProperty targetProperty,
                                     const ModelNode &targetNode,
                                     bool &outMoveNodesAfter);
ModelNode handleItemLibraryFontDrop(const QString &fontFamily,
                                    NodeAbstractProperty targetProperty,
                                    const ModelNode &targetNode);
ModelNode handleItemLibraryShaderDrop(const QString &shaderPath,
                                      bool isFragShader,
                                      NodeAbstractProperty targetProperty,
                                      const ModelNode &targetNode,
                                      bool &outMoveNodesAfter);
ModelNode handleItemLibrarySoundDrop(const QString &soundPath,
                                     NodeAbstractProperty targetProperty,
                                     const ModelNode &targetNode);
ModelNode handleItemLibraryTexture3dDrop(const QString &tex3DPath,
                                         NodeAbstractProperty targetProperty,
                                         const ModelNode &targetNode,
                                         bool &outMoveNodesAfter);

// ModelNodePreviewImageOperations
QVariant previewImageDataForGenericNode(const ModelNode &modelNode);
QVariant previewImageDataForImageNode(const ModelNode &modelNode);

} // namespace ModelNodeOperations
} //QmlDesigner
