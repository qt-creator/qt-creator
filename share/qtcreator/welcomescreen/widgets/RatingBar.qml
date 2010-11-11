import QtQuick 1.0

Row {
    property int rating : 2
    property int totalRating: 3

    Repeater { id: rep1; model: rating; Image { source: "img/face-star.png"; width: 22 } }
    Repeater { id: rep2; model: totalRating-rating; Image { source: "img/draw-star.png"; width: 22 } }
}
