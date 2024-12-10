
set(qan_source_files
    qanBehaviour.cpp
    qanBottomRightResizer.cpp
    qanRightResizer.cpp
    qanBottomResizer.cpp
    qanConnector.cpp
    qanDraggable.cpp
    qanDraggableCtrl.cpp
    qanEdge.cpp
    qanEdgeItem.cpp
    qanEdgeDraggableCtrl.cpp
    qanGraph.cpp
    qanGraphView.cpp
    qanGrid.cpp
    qanLineGrid.cpp
    qanGroup.cpp
    qanGroupItem.cpp
    qanNavigable.cpp
    qanNavigablePreview.cpp
    qanNode.cpp
    qanNodeItem.cpp
    qanPortItem.cpp
    qanSelectable.cpp
    qanStyle.cpp
    qanStyleManager.cpp
    qanAnalysisTimeHeatMap.cpp
    qanUtils.cpp
    qanTableGroup.cpp
    qanTableCell.cpp
    qanTableBorder.cpp
    qanTableGroupItem.cpp
    qanTreeLayouts.cpp
    )

set (qan_header_files
    qanAbstractDraggableCtrl.h
    qanBehaviour.h
    qanBottomRightResizer.h
    qanRightResizer.h
    qanBottomResizer.h
    qanConnector.h
    qanDraggable.h
    qanDraggableCtrl.h
    qanEdge.h
    qanEdgeDraggableCtrl.h
    qanEdgeItem.h
    qanGraph.h
    qanGraphView.h
    qanGrid.h
    qanGroup.h
    qanGroupItem.h
    qanLineGrid.h
    qanNavigable.h
    qanNavigablePreview.h
    qanNode.h
    qanNodeItem.h
    qanPortItem.h
    qanSelectable.h
    qanStyle.h
    qanStyleManager.h
    qanAnalysisTimeHeatMap.cpp
    qanUtils.h
    qanTableGroup.h
    qanTableCell.h
    qanTableBorder.h
    qanTableGroupItem.h
    qanTreeLayouts.h
    QuickQanava.h
    gtpo/container_adapter.h
    gtpo/edge.h
    gtpo/graph.h
    gtpo/graph.hpp
    gtpo/graph_property.h
    gtpo/node.h
    gtpo/node.hpp
    gtpo/observable.h
    gtpo/observer.h
    )

set(quickcontainers_source_files
    quickcontainers/qcmContainerModel.cpp
    quickcontainers/qcmAbstractContainer.cpp
    )

set (quickcontainers_header_files
    quickcontainers/QuickContainers.h
    quickcontainers/qcmContainerModel.h
    quickcontainers/qcmAbstractContainer.h
    quickcontainers/qcmContainer.h
    quickcontainers/qcmAdapter.h
    )


set (qan_plugin_files
    ${CMAKE_CURRENT_SOURCE_DIR}/qanPlugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qanPlugin.h
)

set(qan_qml_files
    NavigablePreview.qml
    GraphPreview.qml
    HeatMapPreview.qml
    LineGrid.qml
    Edge.qml
    EdgeTemplate.qml
    EdgeStraightPath.qml
    EdgeOrthoPath.qml
    EdgeCurvedPath.qml
    EdgeSrcArrowPath.qml
    EdgeSrcCirclePath.qml
    EdgeSrcRectPath.qml
    EdgeDstArrowPath.qml
    EdgeDstCirclePath.qml
    EdgeDstRectPath.qml
    GraphView.qml
    Node.qml
    Port.qml
    VerticalDock.qml
    HorizontalDock.qml
    Group.qml
    TableGroup.qml
    TableCell.qml
    TableBorder.qml
    RectGroupTemplate.qml
    CanvasNodeTemplate.qml
    RectNodeTemplate.qml
    RectSolidBackground.qml
    RectSolidShadowBackground.qml
    RectShadowEffect.qml
    RectSolidGlowBackground.qml
    RectGlowEffect.qml
    RectGradientBackground.qml
    RectGradientShadowBackground.qml
    RectGradientGlowBackground.qml
    SelectionItem.qml
    VisualConnector.qml
    LabelEditor.qml
    OriginCross.qml
)

# Configure Qt
set(CMAKE_AUTOMOC ON)
#qt_wrap_cpp(qan_source_files, qan_header_files)   # Creates .moc files from sources
set(CMAKE_AUTORCC ON)
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:QT_QML_DEBUG>)

# Configure QuickQanava library ###############################################

# FIXME #248
# list(APPEND qan_resources QuickQanava_static.qrc)
# add_library(QuickQanava STATIC
#     ${qan_source_files}
#     ${qan_header_files}
#     ${quickcontainers_source_files}
#     ${quickcontainers_header_files}
#     ${qan_resources}
#     )

# qt_add_library(QuickQanava STATIC)
qt_add_qml_module(QuickQanava
    STATIC
    URI QuickQanava
    # VERSION 2.5
    SOURCES
        ${qan_source_files} ${qan_header_files} ${quickcontainers_source_files} ${quickcontainers_header_files}
    QML_FILES
        ${qan_qml_files}
    RESOURCE_PREFIX /
    #OUTPUT_DIRECTORY QuickQanava
    OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/qtcreator/share/qtcreator/qmldesigner/nodegrapheditor/imports/QuickQanava
    #RESOURCE QuickQanava_static.qrc
)
#set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/QuickQanava)

target_include_directories(QuickQanava
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# target_compile_features(QuickQanava
#     PUBLIC
#         cxx_std_17
#     )

# install(FILES
#     ${qan_header_files}
#     DESTINATION include/quickqanava
#     )

# install(TARGETS QuickQanava
#     EXPORT Targets
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin
#     INCLUDES DESTINATION include/quickqanava
#     )

set(CMAKE_INCLUDE_CURRENT_DIR ON)
target_link_libraries(QuickQanava PUBLIC Qt6::Core
                                         Qt6::Gui
                                         Qt6::Qml
                                         Qt6::Quick
                                         Qt6::QuickControls2)


