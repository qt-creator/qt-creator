
#include "pkgconfigtool.h"
#include <QProcess>
#include <QTextStream>
#include <QtDebug>

using namespace GenericProjectManager::Internal;

PkgConfigTool::PkgConfigTool()
{ }

PkgConfigTool::~PkgConfigTool()
{ }

QList<PkgConfigTool::Package> PkgConfigTool::packages() const
{
    if (m_packages.isEmpty())
        packages_helper();

    return m_packages;
}

void PkgConfigTool::packages_helper() const
{
    QStringList args;
    args.append(QLatin1String("--list-all"));

    QProcess pkgconfig;
    pkgconfig.start(QLatin1String("pkg-config"), args);
    while (! pkgconfig.waitForFinished()) {
    }

    QTextStream in(&pkgconfig);
    forever {
        const QString line = in.readLine();

        if (line.isNull())
            break;

        else if (line.isEmpty())
            continue;

        int i = 0;
        for (; i != line.length(); ++i) {
            if (line.at(i).isSpace())
                break;
        }

        Package package;
        package.name = line.left(i).trimmed();
        package.description = line.mid(i).trimmed();

        QStringList args;
        args << package.name << QLatin1String("--cflags");
        pkgconfig.start(QLatin1String("pkg-config"), args);

        while (! pkgconfig.waitForFinished()) {
        }

        const QString cflags = QString::fromUtf8(pkgconfig.readAll());

        int index = 0;
        while (index != cflags.size()) {
            const QChar ch = cflags.at(index);

            if (ch.isSpace()) {
                do { ++index; }
                while (index < cflags.size() && cflags.at(index).isSpace());
            }

            else if (ch == QLatin1Char('-') && index + 1 < cflags.size()) {
                ++index;

                const QChar opt = cflags.at(index);

                if (opt == QLatin1Char('I')) {
                    // include paths.

                    int start = ++index;
                    for (; index < cflags.size(); ++index) {
                        if (cflags.at(index).isSpace())
                            break;
                    }

                    qDebug() << "*** add include path:" << cflags.mid(start, index - start);
                    package.includePaths.append(cflags.mid(start, index - start));
                }
            }

            else {
                for (; index < cflags.size(); ++index) {
                    if (cflags.at(index).isSpace())
                        break;
                }
            }
        }

        m_packages.append(package);
    }
}
