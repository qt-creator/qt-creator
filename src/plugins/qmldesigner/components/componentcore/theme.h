/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qmldesignercorelib_global.h>

#include <utils/theme/theme.h>

#include <QColor>
#include <QMap>

QT_BEGIN_NAMESPACE
class QQmlEngine;
QT_END_NAMESPACE

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT Theme : public Utils::Theme
{
    Q_OBJECT

    Q_ENUMS(Icon)

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
        aliasAnimated,
        aliasProperty,
        alignBottom,
        alignCenterHorizontal,
        alignCenterVertical,
        alignLeft,
        alignRight,
        alignTo,
        alignTop,
        assign,
        anchorBaseline,
        anchorBottom,
        anchorFill,
        anchorLeft,
        anchorRight,
        anchorTop,
        annotationBubble,
        annotationDecal,
        centerHorizontal,
        centerVertical,
        curveEditor,
        closeCross,
        decisionNode,
        deleteColumn,
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
        edit,
        fontStyleBold,
        fontStyleItalic,
        fontStyleStrikethrough,
        fontStyleUnderline,
        mergeCells,
        redo,
        splitColumns,
        splitRows,
        startNode,
        testIcon,
        textAlignBottom,
        textAlignCenter,
        textAlignLeft,
        textAlignMiddle,
        textAlignRight,
        textAlignTop,
        textBulletList,
        textFullJustification,
        textNumberedList,
        tickIcon,
        triState,
        undo,
        upDownIcon,
        upDownSquare2,
        wildcard
    };

    static Theme *instance();
    static QString replaceCssColors(const QString &input);
    static void setupTheme(QQmlEngine *engine);
    static QColor getColor(Color role);
    static QPixmap getPixmap(const QString &id);
    static QString getIconUnicode(Theme::Icon i);

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
