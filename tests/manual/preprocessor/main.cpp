
#include <PreprocessorEnvironment.h>
#include <PreprocessorClient.h>
#include <pp.h>

#include <QFile>
#include <iostream>

using namespace CPlusPlus;

int main()
{
    Client *client = 0;
    Environment env;
    Preprocessor preprocess(client, &env);

    QFile in;
    if (! in.open(stdin, QFile::ReadOnly))
        return 0;

    const QByteArray source = in.readAll();
    const QByteArray preprocessedCode = preprocess("<stdin>", source);

    std::cout << preprocessedCode.constData();

    return 0;
}
