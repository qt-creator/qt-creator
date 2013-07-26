import qbs

CppApplication {
    type: "application" // To suppress bundle generation on Mac
    files: "main.c"
}
