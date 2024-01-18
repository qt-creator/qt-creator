// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.10

/*!
    \qmltype StringMapper
    \inqmlmodule QtQuick.Studio.LogicHelper
    \since QtQuick.Studio.LogicHelper 1.0
    \inherits QtObject

    \brief Converts numbers to strings with the defined precision.

    The StringMapper type maps numbers to strings. The string mapper
    \l input property is bound to the \c value property of a control that
    provides the number and the \l text property of the string mapper is
    mapped to the \c text property of type that displays the string.

    Designers can use the String Mapper type in \QDS instead of writing
    JavaScript expressions.

    \section1 Example Usage

    In the following example, we use \l Text type to display the numerical
    value of a \l Slider type as a string:

    \code
    Rectangle {
        Slider {
            id: slider
            value: 0.5
        }
        Text {
            id: text1
            text: stringMapper.text
        }
        StringMapper {
            id: stringMapper
            input: slider.value
        }
    }
    \endcode
*/

QtObject {
    id: object

/*!
    The value to convert to a string.
*/
    property real input: 0

/*!
    The number of digits after the decimal separator.
*/
    property int decimals: 2

/*!
    The \l input value as a string.
*/
    property string text: object.input.toFixed(object.decimals)
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
