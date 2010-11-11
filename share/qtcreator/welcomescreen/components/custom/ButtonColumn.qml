import Qt 4.7
import "ButtonGroup.js" as Behavior

/*
   Class: ButtonColumn
   A ButtonColumn allows you to group Buttons in a column. It provides a selection-behavior as well.

   Note: This component don't support the enabled property.
   If you need to disable it you should disable all the buttons inside it.

   <code>
       ButtonColumn {
           Button { text: "Top" }
           Button { text: "Bottom" }
       }
   </code>
*/
Column {
    id: root

    /*
     * Property: exclusive
     * [bool=true] Specifies the grouping behavior. If enabled, the checked property on buttons contained
     * in the group will be exclusive.
     *
     * Note that a button in an exclusive group will allways be checkable
     */
    property bool exclusive: true

    /*
     * Property: checkedButton
     * [string] Contains the last checked Button.
     */
    property Item checkedButton;

    Component.onCompleted: {
        Behavior.create(root, {direction: Qt.Vertical});
    }

    Component.onDestruction: {
        Behavior.destroy();
    }

}
