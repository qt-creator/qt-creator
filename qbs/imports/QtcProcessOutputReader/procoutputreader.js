var Process = loadExtension("qbs.Process")

function readOutput(executable, args)
{
    var p = new Process();
    var output = "";
    if (p.exec(executable, args, false) !== -1)
        output = p.readStdOut().trim(); // Trailing newline.
    p.close();
    return output;
}
