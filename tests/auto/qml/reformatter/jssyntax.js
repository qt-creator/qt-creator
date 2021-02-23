var x
var y = 12

var a_var = 1
let a_let = 2
const a_const = 3

const tmpl = `template` + `t${i + 6}` + `t${i + `nested${i}`}` + `t${function () {
    return 5
}()}` + `t\${i}
${i + 2}`

function foo(a, b) {
    x = 15
    x += 4
}

var foo = function (a, b = 0) {}

function spread() {
    iterableObj = [1, 2]
    obj = {
        "a": 42
    }
    foo(...iterableObj)
    let arr = [...iterableObj, '4', 'five', 6]
    foo(-1, ...args, 2, ...[3])
    console.log(Math.max(...[1, 2, 3, 4]))
}

const func1 = x => x * 2
const func2 = x => {
    return x * 7
}
const func3 = (x, y) => x + y
const func4 = (x, y) => {
    return x + y
}

const s1 = `test`
const s2 = `${42 * 1}`
const s3 = `test ${s2}`
const s4 = `${s2}${s3}test`

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
