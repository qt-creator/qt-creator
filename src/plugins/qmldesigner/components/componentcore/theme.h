// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercomponents_global.h>

#include <utils/theme/theme.h>

#include <QColor>
#include <QMap>

QT_BEGIN_NAMESPACE
class QQmlEngine;
QT_END_NAMESPACE

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT Theme : public Utils::Theme
{
    Q_OBJECT
public:
    enum Icon {
        actionIcon,
        actionIconBinding,
        addColumnAfter,
        addColumnBefore,
        addFile,
        addRowAfter,
        addRowBefore,
        addTable,
        adsClose,
        adsDetach,
        adsDropDown,
        alias,
        aliasAnimated,
        alignBottom,
        alignCenterHorizontal,
        alignCenterVertical,
        alignLeft,
        alignRight,
        alignTo,
        alignTop,
        anchorBaseline,
        anchorBottom,
        anchorFill,
        anchorLeft,
        anchorRight,
        anchorTop,
        animatedProperty,
        annotationBubble,
        annotationDecal,
        applyMaterialToSelected,
        assign,
        bevelAll,
        bevelCorner,
        centerHorizontal,
        centerVertical,
        closeCross,
        closeLink,
        colorPopupClose,
        columnsAndRows,
        copyLink,
        copyStyle,
        cornerA,
        cornerB,
        cornersAll,
        curveDesigner,
        curveEditor,
        customMaterialEditor,
        decisionNode,
        deleteColumn,
        deleteMaterial,
        deleteRow,
        deleteTable,
        detach,
        distributeBottom,
        distributeCenterHorizontal,
        distributeCenterVertical,
        distributeLeft,
        distributeOriginBottomRight,
        distributeOriginCenter,
        distributeOriginNone,
        distributeOriginTopLeft,
        distributeRight,
        distributeSpacingHorizontal,
        distributeSpacingVertical,
        distributeTop,
        download,
        downloadUnavailable,
        downloadUpdate,
        downloaded,
        edit,
        eyeDropper,
        favorite,
        flowAction,
        flowTransition,
        fontStyleBold,
        fontStyleItalic,
        fontStyleStrikethrough,
        fontStyleUnderline,
        gradient,
        gridView,
        idAliasOff,
        idAliasOn,
        infinity,
        keyframe,
        linkTriangle,
        linked,
        listView,
        lockOff,
        lockOn,
        materialPreviewEnvironment,
        materialPreviewModel,
        mergeCells,
        minus,
        mirror,
        newMaterial,
        openLink,
        openMaterialBrowser,
        orientation,
        paddingEdge,
        paddingFrame,
        pasteStyle,
        pause,
        pin,
        play,
        plus,
        promote,
        readOnly,
        redo,
        rotationFill,
        rotationOutline,
        search,
        sectionToggle,
        splitColumns,
        splitRows,
        startNode,
        testIcon,
        textAlignBottom,
        textAlignCenter,
        textAlignJustified,
        textAlignLeft,
        textAlignMiddle,
        textAlignRight,
        textAlignTop,
        textBulletList,
        textFullJustification,
        textNumberedList,
        tickIcon,
        translationCreateFiles,
        translationCreateReport,
        translationExport,
        translationImport,
        translationSelectLanguages,
        translationTest,
        transparent,
        triState,
        triangleArcA,
        triangleArcB,
        triangleCornerA,
        triangleCornerB,
        unLinked,
        undo,
        unpin,
        upDownIcon,
        upDownSquare2,
        visibilityOff,
        visibilityOn,
        wildcard,
        wizardsAutomotive,
        wizardsDesktop,
        wizardsGeneric,
        wizardsMcuEmpty,
        wizardsMcuGraph,
        wizardsMobile,
        wizardsUnknown,
        zoomAll,
        zoomIn,
        zoomOut,
        zoomSelection
    };
    Q_ENUM(Icon)

    static Theme *instance();
    static QString replaceCssColors(const QString &input);
    static void setupTheme(QQmlEngine *engine);
    static QColor getColor(Color role);
    static QPixmap getPixmap(const QString &id);
    static QString getIconUnicode(Theme::Icon i);
    static QString getIconUnicode(const QString &name);

    Q_INVOKABLE QColor qmlDesignerBackgroundColorDarker() const;
    Q_INVOKABLE QColor qmlDesignerBackgroundColorDarkAlternate() const;
    Q_INVOKABLE QColor qmlDesignerTabLight() const;
    Q_INVOKABLE QColor qmlDesignerTabDark() const;
    Q_INVOKABLE QColor qmlDesignerButtonColor() const;
    Q_INVOKABLE QColor qmlDesignerBorderColor() const;

    Q_INVOKABLE int smallFontPixelSize() const;
    Q_INVOKABLE int captionFontPixelSize() const;
    Q_INVOKABLE bool highPixelDensity() const;

private:
    Theme(Utils::Theme *originTheme, QObject *parent);
    QColor evaluateColorAtThemeInstance(const QString &themeColorName);

    QObject *m_constants;
};

} // namespace QmlDesigner
