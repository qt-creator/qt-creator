var x
var y = 12

var a_var = 1
let a_let = 2
const a_const = 3

function foo(a, b) {
    x = 15
    x += 4
}

var foo = function (a, b) {}

while (true) {
    for (var a = 1; a < 5; ++a) {
        switch (a) {
        case 1:
            ++a
            break
        case 2:
            a += 2
            foo()
            break
        default:
            break
        case 3:
            continue
        }
    }

    for (var x in a) {
        print(a[x])
    }
    for (let x in a) {
        print(a[x])
    }
    for (const x in a) {
        print(a[x])
    }

    do {
        a = x
        x *= a
    } while (a < x)

    try {
        Math.sqrt(a)
    } catch (e) {
        Math.sqrt(a)
    } finally {
        Math.sqrt(a)
    }

    try {
        Math.sqrt(a)
    } finally {
        Math.sqrt(a)
    }

    try {
        Math.sqrt(a)
    } catch (e) {
        Math.sqrt(a)
    }
}
