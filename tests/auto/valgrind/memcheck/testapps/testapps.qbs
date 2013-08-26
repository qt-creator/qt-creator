import qbs

Project {
    name: "Memcheck test apps"
    references: [
        "free1/free1.qbs",
        "free2/free2.qbs",
        "invalidjump/invalidjump.qbs",
        "leak1/leak1.qbs",
        "leak2/leak2.qbs",
        "leak3/leak3.qbs",
        "leak4/leak4.qbs",
        "overlap/overlap.qbs",
        "syscall/syscall.qbs",
        "uninit1/uninit1.qbs",
        "uninit2/uninit2.qbs",
        "uninit3/uninit3.qbs"
    ]
}
