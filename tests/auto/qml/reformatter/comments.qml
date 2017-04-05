import QtQuick 2.0


/* Comment 1
   This is a multiline comment. */

/*
 Another multiline comment.

 A slightly different formatting.
*/
Item {
    /*
      Indented multiline comment.
      */

    // Comment over a signal.
    signal foo

    function test() {
        for (var i = model.count - 1; i >= 0; --i) // in-line comment
        {
            console.log("test")
        }
    }
}
