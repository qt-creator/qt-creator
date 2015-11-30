
While the primary intention of this pretty printing implementation is
to provide what Qt Creator needs, it can be used in a plain commandline
GDB session, too.

With

        python sys.path.insert(1, '<path/to/qtcreator>/share/qtcreator/debugger/')
        python from gdbbridge import *

in .gdbinit there is a new "GDB command", called "pp".

With code like

        int main(int argc, char *argv[])
        {
            QString ss = "Hello";
            QApplication app(argc, argv);
            app.setObjectName(ss);
            // break here
        }

    the "pp" command can be used as follows:

    (gdb) pp app
    app =
       [
          <QGuiApplication> = {"Hello"}
          staticMetaObject = <QMetaObject> = {""}
          [parent] = <QObject *> = {"0x0"}
          [children] = <QObjectList> = {"<3 items>"}
          [properties] = "<>0 items>"
          [methods] = "<6 items>"
          [signals] = "<1 items>"
       ],<QApplication> = {"Hello"}

    (gdb) pp app [properties],[children]
    app =
       [
          <QGuiApplication> = {"Hello"}
          staticMetaObject = <QMetaObject> = {""}
          [parent] = <QObject *> = {"0x0"}
          [children] = [
             <QObject> = {""}
             <QObject> = {""}
             <QObject> = {"fusion"}
          ],<QObjectList> = {"<3 items>"}
          [properties] = [
             windowIcon = <QVariant (QIcon)> = {""}
             cursorFlashTime = <QVariant (int)> = {"1000"}
             doubleClickInterval = <QVariant (int)> = {"400"}
             keyboardInputInterval = <QVariant (int)> = {"400"}
             wheelScrollLines = <QVariant (int)> = {"3"}
             globalStrut = <QVariant (QSize)> = {"(0, 0)"}
             startDragTime = <QVariant (int)> = {"500"}
             startDragDistance = <QVariant (int)> = {"10"}
             styleSheet = <QVariant (QString)> = {""}
             autoSipEnabled = <QVariant (bool)> = {"true"}
          ],"<10 items>"
          [methods] = "<6 items>"
          [signals] = "<1 items>"
       ],<QApplication> = {"Hello"}

    (gdb) pp ss
    ss =
       <QString> = {"Hello"}


