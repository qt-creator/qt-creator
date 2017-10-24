
import sys
#from PyQt4 import QtGui



def testApp():
    app = QtGui.QApplication(sys.argv)

    class MessageBox(QtGui.QWidget):
        def __init__(self, parent=None):
            QtGui.QWidget.__init__(self, parent)
            self.setGeometry(200, 200, 300, 200)
            self.setWindowTitle("A Test Box")

    messageBox = MessageBox()
    messageBox.show()

    widget = QtGui.QWidget()
    widget.resize(200, 200)
    widget.show()

    return app.exec_()


def testMath():
    def square(a):
        x = a * a
        return a

    def cube(a):
        l = [1, 2, 4]
        t = (1, 2, 3)
        d = {1: 'one', 2: 'two', 'three': 3}
        s = u'unixcode'
        x = range(1, 10)
#        b = buffer("xxx")
        x = square(a)
        x = x * a
        x = x + 1
        x = x - 1
        return x

    print(cube(3))
    print(cube(4))
    print(cube(5))

def main():
    testMath()
    #testApp()
    return 0

if __name__ == '__main__':
    sys.exit(main())
