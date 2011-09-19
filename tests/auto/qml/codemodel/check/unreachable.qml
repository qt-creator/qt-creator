import Qt 4.7
// DEFAULTMSG unreachable
Item {
    function foo() {
        return
        x() // W 9 11
        x()
    }

    function foo() {
        throw new Object()
        x() // W 9 11
        x()
    }

    function foo() {
        if (a)
            return
        x()
        if (a)
            foo();
        else
            return
        x()
        if (a)
            return
        else
            return
        x() // W 9 11
    }

    function foo() {
        try {
            throw 1
        } finally {}
        x() // W 9 11
    }

    function foo() {
        try {
        } finally {
            return
        }
        x() // W 9 11
    }

    function foo() {
        try {
        } catch(a) {
            return
        }
        x()
        try {
            return
        } catch(a) {
        }
        x()
        try {
            return
        } catch(a) {
            return
        }
        x() // W 9 11
    }

    function foo() {
        switch (a) {
        case 0:
            break
        case 1:
        case 2:
            return
        }
        x()
        switch (a) {
        case 1:
        case 2:
            return
        }
        x()
        switch (a) {
        case 1:
        case 2:
            return
        default:
            return
        }
        x() // W 9 11
    }

    function foo() {
        l1: do {
            l2: while (b) {
                return
            }
            x()
            l3: do {
                break l1
            } while (b);
            x() // W 13 15
        } while (a);
        x() // reachable via break
    }
}
