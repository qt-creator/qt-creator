/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Controls.Private 1.0

/*!
    \qmltype ComboBox
    \inqmlmodule QtQuick.Controls
    \since 5.1
    \ingroup controls
    \brief Provides a drop-down list functionality.

    Add items to the comboBox by assigning it a ListModel, or a list of strings to the \l model property.

    \qml
       ComboBox {
           width: 200
           model: [ "Banana", "Apple", "Coconut" ]
       }
    \endqml

    In this example we are demonstrating how to use a ListModel with a combo box.

    \qml
       ComboBox {
           currentIndex: 2
           model: ListModel {
               id: cbItems
               ListElement { text: "Banana"; color: "Yellow" }
               ListElement { text: "Apple"; color: "Green" }
               ListElement { text: "Coconut"; color: "Brown" }
           }
           width: 200
           onCurrentIndexChanged: console.debug(cbItems.get(currentIndex).text + ", " + cbItems.get(currentIndex).color)
       }
    \endqml

    You can make a combo box editable by setting the \l editable property. An editable combo box will
    autocomplete its text based on what is available in the model.

    In the next example we demonstrate how you can append content to an editable combo box by
    reacting to the \l accepted signal. Note that you have to explicitly prevent duplicates.

    \qml
        ComboBox {
            editable: true
            model: ListModel {
                id: model
                ListElement { text: "Banana"; color: "Yellow" }
                ListElement { text: "Apple"; color: "Green" }
                ListElement { text: "Coconut"; color: "Brown" }
            }
            onAccepted: {
                if (editableCombo.find(currentText) === -1) {
                    model.append({text: editText})
                    currentIndex = editableCombo.find(editText)
                }
            }
        }
    \endqml


    You can create a custom appearance for a ComboBox by
    assigning a \l {QtQuick.Controls.Styles::ComboBoxStyle}{ComboBoxStyle}.
*/

Control {
    id: comboBox

    /*! \qmlproperty model ComboBox::model
        The model to populate the ComboBox from.

        Changing the model after initialization will reset \l currentIndex to \c 0.
    */
    property alias model: popupItems.model

    /*! The model role used for populating the ComboBox. */
    property string textRole: ""

    /*! \qmlproperty int ComboBox::currentIndex
        The index of the currently selected item in the ComboBox.

        \sa model
    */
    property alias currentIndex: popup.__selectedIndex

    /*! \qmlproperty string ComboBox::currentText
        The text of the currently selected item in the ComboBox.

        \note Since \c currentText depends on \c currentIndex, there's no way to ensure \c currentText
        will be up to date whenever a \c onCurrentIndexChanged handler is called.
    */
    readonly property alias currentText: popup.currentText

    /*! This property holds whether the combo box can be edited by the user.
     The default value is \c false.
     \since QtQuick.Controls 1.1
    */
    property bool editable: false

    /*! \qmlproperty string ComboBox::editText
        \since QtQuick.Controls 1.1
        This property specifies text being manipulated by the user for an editable combo box.
    */
    property alias editText: input.text

    /*! This property specifies whether the combobox should gain active focus when pressed.
        The default value is \c false. */
    property bool activeFocusOnPress: false

    /*! \qmlproperty bool ComboBox::pressed

        This property holds whether the button is being pressed. */
    readonly property bool pressed: mouseArea.pressed && mouseArea.containsMouse || popup.__popupVisible

    /*! \qmlproperty bool ComboBox::hovered

        This property indicates whether the control is being hovered.
    */
    readonly property alias hovered: mouseArea.containsMouse

    /*! \qmlproperty int ComboBox::count
        \since QtQuick.Controls 1.1
        This property holds the number of items in the combo box.
    */
    readonly property alias count: popupItems.count

    /*! Returns the text for a given \a index.
        If an invalid index is provided, \c null is returned
        \since QtQuick.Controls 1.1
    */
    function textAt (index) {
        if (index >= count || index < 0)
            return null;
        return popupItems.objectAt(index).text;
    }

    /*! Finds and returns the index of a given \a text
        If no match is found, \c -1 is returned. The search is case sensitive.
        \since QtQuick.Controls 1.1
    */
    function find (text) {
        return input.find(text, Qt.MatchExactly)
    }

    /*!
        \qmlproperty Validator ComboBox::validator
        \since QtQuick.Controls 1.1

        Allows you to set a text validator for an editable ComboBox.
        When a validator is set,
        the text field will only accept input which leaves the text property in
        an intermediate state. The accepted signal will only be sent
        if the text is in an acceptable state when enter is pressed.

        Currently supported validators are \l{QtQuick2::IntValidator},
        \l{QtQuick2::DoubleValidator}, and \l{QtQuick2::RegExpValidator}. An
        example of using validators is shown below, which allows input of
        integers between 11 and 31 into the text field:

        \note This property is only applied when \l editable is \c true

        \qml
        import QtQuick 2.1
        import QtQuick.Controls 1.1

        ComboBox {
            editable: true
            model: 10
            validator: IntValidator {bottom: 0; top: 10;}
            focus: true
        }
        \endqml

        \sa acceptableInput, accepted, editable
    */
    property alias validator: input.validator

    /*!
        \qmlproperty bool ComboBox::acceptableInput
        \since QtQuick.Controls 1.1

        Returns \c true if the combo box contains acceptable
        text in the editable text field.

        If a validator was set, this property will return \c
        true if the current text satisfies the validator or mask as
        a final string (not as an intermediate string).

        \sa validator, accepted

    */
    readonly property alias acceptableInput: input.acceptableInput

    /*!
        \qmlsignal ComboBox::accepted()
        \since QtQuick.Controls 1.1

        This signal is emitted when the Return or Enter key is pressed on an
        \l editable combo box. If the confirmed string is not currently in the model,
        the currentIndex will be set to -1 and the \l currentText will be updated
        accordingly.

        \note If there is a \l validator set on the combobox,
        the signal will only be emitted if the input is in an acceptable state.
    */
    signal accepted

    /*!
        \qmlsignal ComboBox::activated(int index)
        \since QtQuick.Controls 1.1

        \a index is the triggered model index or -1 if a new string is accepted

        This signal is similar to currentIndex changed, but will only
        be emitted if the combo box index was changed by the user and not
        when set programatically.
    */
    signal activated(int index)

    /*!
        \qmlmethod ComboBox::selectAll()
        \since QtQuick.Controls 1.1

        Causes all \l editText to be selected.
    */
    function selectAll() {
        input.selectAll()
    }

    /*! \internal */
    property var __popup: popup

    style: Qt.createComponent(Settings.style + "/ComboBoxStyle.qml", comboBox)

    activeFocusOnTab: true

    Accessible.role: Accessible.ComboBox

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onPressed: {
            if (comboBox.activeFocusOnPress)
                forceActiveFocus()
            popup.show()
        }
    }

    Component.onCompleted: {
        if (currentIndex === -1)
            currentIndex = 0

        popup.ready = true
        popup.resolveTextValue(textRole)
    }

    Keys.onPressed: {
        // Perform one-character based lookup for non-editable combo box
        if (!editable && event.text.length > 0) {
            var index = input.find(event.text, Qt.MatchStartsWith);
            if (index >= 0 && index !== currentIndex) {
                currentIndex = index;
                activated(currentIndex);
            }
        }
    }

    TextInput {
        id: input

        visible: editable
        enabled: editable
        focus: true
        clip: contentWidth > width
        text: currentText

        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: __style.drowDownButtonWidth

        verticalAlignment: Text.AlignVCenter

        renderType: Text.NativeRendering
        selectByMouse: true
        selectionColor: syspal.highlight
        selectedTextColor: syspal.highlightedText
        onAccepted: {
            var idx = input.find(editText)
            if (idx > -1) {
                var string = textAt(idx);
                if (string.length === editText.length) {
                    currentIndex = idx;
                    editText = string;
                }
            } else {
                currentIndex = -1;
                popup.currentText = editText;
            }
            comboBox.accepted();
        }

        SystemPalette { id: syspal }

        property bool blockUpdate: false
        property string prevText

        function find (text, searchType) {
            for (var i = 0 ; i < popupItems.count ; ++i) {
                var currentString = popupItems.objectAt(i).text
                if (searchType === Qt.MatchExactly) {
                    if (text === currentString)
                        return i;
                } else if (searchType === Qt.CaseSensitive) {
                    if (currentString.indexOf(text) === 0)
                        return i;
                } else if (currentString.toLowerCase().indexOf(text.toLowerCase()) === 0) {
                    return i
                }
            }
            return -1;
        }

        // Finds first entry and shortest entry. Used by editable combo
        function tryComplete (inputText) {
            var candidate = "";
            var shortestString = "";
            for (var i = 0 ; i < popupItems.count ; ++i) {
                var currentString = popupItems.objectAt(i).text;

                if (currentString.toLowerCase().indexOf(inputText.toLowerCase()) === 0) {
                    if (candidate.length) { // Find smallest possible match
                        var cmp = 0;

                        // We try to complete the shortest string that matches our search
                        if (currentString.length < candidate.length)
                            candidate = currentString

                        while (cmp < Math.min(currentString.length, shortestString.length)
                               && shortestString[cmp].toLowerCase() === currentString[cmp].toLowerCase())
                            cmp++;
                        shortestString = shortestString.substring(0, cmp);
                    } else { // First match, select as current index and find other matches
                        candidate = currentString;
                        shortestString = currentString;
                    }
                }
            }

            if (candidate.length)
                return inputText + candidate.substring(inputText.length, candidate.length);
            return inputText;
        }

        property bool allowComplete: false
        Keys.onPressed: allowComplete = (event.key !== Qt.Key_Backspace && event.key !== Qt.Key_Delete);

        onTextChanged: {
            if (editable && !blockUpdate && allowComplete) {
                var completed = input.tryComplete(text)
                if (completed.length > text.length) {
                    var oldtext = input.text;
                    input.text = completed;
                    input.select(text.length, oldtext.length);
                }
            }
            prevText = text
        }
    }

    onTextRoleChanged: popup.resolveTextValue(textRole)

    Menu {
        id: popup
        objectName: "popup"

        style: isPopup ? __style.__popupStyle : __style.__dropDownStyle

        property string currentText: selectedText
        onSelectedTextChanged: if (selectedText) popup.currentText = selectedText

        property string selectedText
        on__SelectedIndexChanged: updateSelectedText()
        property string textRole: ""

        property bool ready: false
        property bool isPopup: !editable && !!__panel && __panel.popup

        property int y: isPopup ? (comboBox.__panel.height - comboBox.__panel.implicitHeight) / 2.0 : comboBox.__panel.height
        __minimumWidth: comboBox.width
        __visualItem: comboBox

        property ExclusiveGroup eg: ExclusiveGroup { id: eg }

        property bool __modelIsArray: popupItems.model ? popupItems.model.constructor === Array : false

        Instantiator {
            id: popupItems
            active: false

            property bool updatingModel: false
            onModelChanged: {
                if (active) {
                    if (updatingModel && popup.__selectedIndex === 0) {
                        // We still want to update the currentText
                        popup.updateSelectedText()
                    } else {
                        updatingModel = true
                        popup.__selectedIndex = 0
                    }
                }
            }

            MenuItem {
                text: popup.textRole === '' ?
                        modelData :
                          ((popup.__modelIsArray ? modelData[popup.textRole] : model[popup.textRole]) || '')
                onTriggered: {
                    if (index !== currentIndex)
                        activated(index)
                    comboBox.editText = text
                }
                checkable: true
                exclusiveGroup: eg
            }
            onObjectAdded: {
                popup.insertItem(index, object)
                if (!updatingModel && index === popup.__selectedIndex)
                    popup.selectedText = object["text"]
            }
            onObjectRemoved: popup.removeItem(object)

        }

        function resolveTextValue(initialTextRole) {
            if (!ready || !model) {
                popupItems.active = false
                return;
            }

            var get = model['get'];
            if (!get && popup.__modelIsArray) {
                if (model[0].constructor !== String && model[0].constructor !== Number)
                    get = function(i) { return model[i]; }
            }

            var modelMayHaveRoles = get !== undefined
            textRole = initialTextRole
            if (textRole === "" && modelMayHaveRoles && get(0)) {
                // No text role set, check whether model has a suitable role
                // If 'text' is found, or there's only one role, pick that.
                var listElement = get(0)
                var roleName = ""
                var roleCount = 0
                for (var role in listElement) {
                    if (listElement[role].constructor === Function)
                        continue;
                    if (role === "text") {
                        roleName = role
                        break
                    } else if (!roleName) {
                        roleName = role
                    }
                    ++roleCount
                }
                if (roleCount > 1 && roleName !== "text") {
                    console.warn("No suitable 'textRole' found for ComboBox.")
                } else {
                    textRole = roleName
                }
            }

            if (!popupItems.active)
                popupItems.active = true
            else
                updateSelectedText()
        }

        function show() {
            if (items[__selectedIndex])
                items[__selectedIndex].checked = true
            __currentIndex = comboBox.currentIndex
            if (Qt.application.layoutDirection === Qt.RightToLeft)
                __popup(comboBox.width, y, isPopup ? __selectedIndex : 0)
            else
                __popup(0, y, isPopup ? __selectedIndex : 0)
        }

        function updateSelectedText() {
            var selectedItem;
            if (__selectedIndex !== -1 && (selectedItem = items[__selectedIndex]))
                selectedText = selectedItem.text
        }
    }

    // The key bindings below will only be in use when popup is
    // not visible. Otherwise, native popup key handling will take place:
    Keys.onSpacePressed: {
        if (!popup.popupVisible)
            popup.show()
    }

    Keys.onUpPressed: {
        input.blockUpdate = true
        if (currentIndex > 0) {
            currentIndex--;
            input.text = popup.currentText;
            activated(currentIndex);
        }
        input.blockUpdate = false;
    }

    Keys.onDownPressed: {
        input.blockUpdate = true;
        if (currentIndex < popupItems.count - 1) {
            currentIndex++;
            input.text = popup.currentText;
            activated(currentIndex);
        }
        input.blockUpdate = false;
    }
}
