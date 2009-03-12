#ifndef PKGCONFIGTOOL_H
#define PKGCONFIGTOOL_H

#include <QObject>
#include <QStringList>

namespace GenericProjectManager {
namespace Internal {

class PkgConfigTool: public QObject
{
    Q_OBJECT

public:
    struct Package {
        QString name;
        QString description;
        QStringList includePaths;
        QStringList defines;
        QStringList undefines;
    };

public:
    PkgConfigTool();
    virtual ~PkgConfigTool();

    QList<Package> packages() const;

private:
    void packages_helper() const;

private:
    mutable QList<Package> _packages;
};

} // end of namespace Internal
} // end of namespace GenericProjectManager

#endif // PKGCONFIGTOOL_H
