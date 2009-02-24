
#include <QtCore>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    args.removeFirst();

    if (args.size() != 3) {
        std::cerr << "Usage: qpatch file oldQtDir newQtDir" << std::endl;
        return EXIT_FAILURE;
    }

    const QString files = args.takeFirst();
    const QByteArray qtDirPath = QFile::encodeName(args.takeFirst());
    const QByteArray newQtPath = QFile::encodeName(args.takeFirst());

    QString suffix;
    if (! args.isEmpty())
        suffix = args.takeFirst();

    if (qtDirPath.size() < newQtPath.size()) {
        std::cerr << "qpatch: error: newQtDir needs to be less than " << qtDirPath.size() << " characters."
                << std::endl;
        return EXIT_FAILURE;
    }

    QFile fn(files);
    if (! fn.open(QFile::ReadOnly)) {
        std::cerr << "qpatch: error: file not found" << std::endl;
        return EXIT_FAILURE;
    }

    QStringList filesToPatch;
    QTextStream in(&fn);
    forever {
        QString line;
        line = in.readLine();

        if (line.isNull())
            break;

        filesToPatch.append(line);
    }


    foreach (QString fileName, filesToPatch) {

        QString prefix;
        prefix += newQtPath;
        if (! prefix.endsWith(QLatin1Char('/')))
            prefix += QLatin1Char('/');

        fileName.prepend(prefix);

        qDebug() << "patch file:" << fileName;
        continue;

        QFile file(fileName);
        if (! file.open(QFile::ReadOnly)) {
            std::cerr << "qpatch: warning: file not found" << std::endl;
            continue;
        }

        const QFile::Permissions permissions = file.permissions();

        const QByteArray source = file.readAll();
        file.close();
        int index = 0;

        QVector<char> patched;

        forever {
            int start = source.indexOf(qtDirPath, index);
            if (start == -1)
                break;

            int endOfString = start;
            while (source.at(endOfString))
                ++endOfString;

            ++endOfString; // include the '\0'

            //qDebug() << "*** found string:" << source.mid(start, endOfString - start);

            for (int i = index; i < start; ++i)
                patched.append(source.at(i));

            int length = endOfString - start;
            QVector<char> s;

            for (const char *x = newQtPath.constData(); x != newQtPath.constEnd() - 1; ++x)
                s.append(*x);

            const int qtDirPathLength = qtDirPath.size();

            for (const char *x = source.constData() + start + qtDirPathLength - 1;
            x != source.constData() + endOfString; ++x)
                s.append(*x);

            const int oldSize = s.size();

            for (int i = oldSize; i < length; ++i)
                s.append('\0');

            for (int i = 0; i < s.size(); ++i)
                patched.append(s.at(i));

            index = endOfString;
        }

        for (int i = index; i < source.size(); ++i)
            patched.append(source.at(i));

        QFile out(fileName /* + suffix*/);
        out.setPermissions(permissions);
        if (out.open(QFile::WriteOnly)) {
            out.write(patched.constData(), patched.size());
        }
    }

    return 0;
}
