#ifndef MERCURIALCONTROL_H
#define MERCURIALCONTROL_H

#include <coreplugin/iversioncontrol.h>

namespace Mercurial {
namespace Internal {

class MercurialClient;

//Implements just the basics of the Version Control Interface
//MercurialClient handles all the work
class MercurialControl: public Core::IVersionControl
{
    Q_OBJECT
public:
    explicit MercurialControl(MercurialClient *mercurialClient);

    QString name() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    bool managesDirectory(const QString &filename) const;
    QString findTopLevelForDirectory(const QString &directory) const;
    bool supportsOperation(Operation operation) const;
    bool vcsOpen(const QString &fileName);
    bool vcsAdd(const QString &filename);
    bool vcsDelete(const QString &filename);
    bool sccManaged(const QString &filename);

signals:
    void enabledChanged(bool);

private:
    MercurialClient *mercurialClient;
    bool mercurialEnabled;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALCONTROL_H
