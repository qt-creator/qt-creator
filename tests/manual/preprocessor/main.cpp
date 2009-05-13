
#include <PreprocessorEnvironment.h>
#include <PreprocessorClient.h>
#include <pp.h>

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QDir>
#include <QtDebug>

#include <iostream>

using namespace CPlusPlus;

class MakeDepend: public Client
{
    Environment *env;
    QList<QDir> systemDirs;

public:
    MakeDepend(Environment *env)
        : env(env)
    { }

    void addSystemDir(const QDir &dir)
    { systemDirs.append(dir); }

    void addSystemDir(const QString &path)
    { systemDirs.append(QDir(path)); }

    virtual void macroAdded(const Macro &)
    { }

    virtual void sourceNeeded(QString &fileName, IncludeType mode, unsigned)
    {
        if (mode == IncludeLocal) {
            // ### cache
            const QFileInfo currentFile(QFile::decodeName(env->currentFile));
            const QDir dir = currentFile.dir();

            QFileInfo fileInfo(dir, fileName);
            if (fileInfo.exists()) {
                fileName = fileInfo.absoluteFilePath();
                std::cout << ' ' << qPrintable(fileName);
                return;
            }
        }

        foreach (const QDir &dir, systemDirs) {
            QFileInfo fileInfo(dir, fileName);
            if (fileInfo.exists() && fileInfo.isFile()) {
                fileName = fileInfo.absoluteFilePath();
                std::cout << ' ' << qPrintable(fileName);
                return;
            }
        }

        std::cerr << "file '" << qPrintable(fileName) << "' not found" << std::endl;
    }

    virtual void startExpandingMacro(unsigned, const Macro &,
                                     const QByteArray &,
                                     const QVector<MacroArgumentReference> &)
    { }

    virtual void stopExpandingMacro(unsigned, const Macro &)
    { }

    virtual void startSkippingBlocks(unsigned)
    { }

    virtual void stopSkippingBlocks(unsigned)
    { }
};

int main(int argc, char *argv[])
{
    Environment env;
    MakeDepend client(&env);

    client.addSystemDir(QLatin1String("/usr/include"));
    Preprocessor preproc(&client, &env);

    for (int i = 1; i < argc; ++i) {
        const QByteArray fileName = argv[i];
        std::cout << fileName.constData() << ':';
        QFile file(QFile::decodeName(fileName));
        if (file.open(QFile::ReadOnly)) {
            // ### we should QTextStream here.
            const QByteArray code = file.readAll();
            preproc.preprocess(fileName, code, /*result = */ 0);
        }
        std::cout << std::endl;
    }

    return 0;
}
