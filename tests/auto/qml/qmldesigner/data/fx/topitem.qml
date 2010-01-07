import Qt 4.6
import Qt 4.6 as Qt46
import "subitems"
import "subitems" as Subdir

Qt46.Rectangle {
    id : "toprect"
    width : 100
    height : 50
    SubItem {
        text: "item1"
    }
    Subdir.SubItem {
        text : "item2"
        y: 25
    }
}
