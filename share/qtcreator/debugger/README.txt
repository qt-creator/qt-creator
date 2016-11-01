
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




In order to hook a new debugger backend into this "common pretty printing system",
the backend should expose a Python API containing at least the following:


class Value:
    name() -> string                      # Name of this thing or None
    type() -> Type                        # Type of this value
    asBytes() -> bytes                    # Memory contents of this object, or None
    address() -> int                      # Address of this object, or None
    dereference() -> Value                # Dereference if value is pointer,
                                          # remove reference if value is reference.
    hasChildren() -> bool                 # Whether this object has subobjects.
    expand() -> bool                      # Make sure that children are accessible.
    nativeDebuggerValue() -> string       # Dumper value returned from the debugger

    childFromName(string name) -> Value   # (optional)
    childFromField(Field field) -> Value  # (optional)
    childFromIndex(int position) -> Value # (optional)


class Type:
    name() -> string                      # Full name of this type
    bitsize() -> int                      # Size of type in bits
    code() -> TypeCodeTypedef
            | TypeCodeStruct
            | TypeCodeVoid
            | TypeCodeIntegral
            | TypeCodeFloat
            | TypeCodeEnum
            | TypeCodePointer
            | TypeCodeArray
            | TypeCodeComplex
            | TypeCodeReference
            | TypeCodeFunction
            | TypeCodeMemberPointer
            | TypeCodeUnresolvable

    unqualified() -> Type                 # Type without const/volatile
    target() -> Type                      # Type dereferenced if it is a pointer type, element if array etc
    stripTypedef() -> Type                # Type with typedefs removed
    fields() -> [ Fields ]                # List of fields (member and base classes) of this type

    templateArgument(int pos, bool numeric) -> Type or int   # (optional)

class Field:
    name() -> string                      # Name of member, None for anonymous items
    isBaseClass() -> bool                 # Whether this is a base class or normal member
    type() -> Type                        # Type of this member
    parentType() -> Type                  # Type of class this member belongs to
    bitsize() -> int                      # Size of member in bits
    bitpos() -> int                       # Offset of member in parent type in bits



parseAndEvaluate(string: expr) -> Value   # or None if not possible.
lookupType(string: name) -> Type          # or None if not possible.
listOfLocals() -> [ Value ]               # List of items currently in scope.
readRawMemory(ULONG64 address, ULONG size) -> bytes # Read a block of data from the virtual address space




