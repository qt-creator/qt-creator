#include "mercurialcontrol.h"
#include "mercurialclient.h"

#include <QtCore/QFileInfo>

using namespace Mercurial::Internal;

MercurialControl::MercurialControl(MercurialClient *client)
        :   mercurialClient(client),
            mercurialEnabled(true)
{
}

QString MercurialControl::name() const
{
    return tr("Mercurial");
}

bool MercurialControl::isEnabled() const
{
    return mercurialEnabled;
}

void MercurialControl::setEnabled(bool enabled)
{
    if (mercurialEnabled != enabled) {
        mercurialEnabled = enabled;
        emit enabledChanged(mercurialEnabled);
    }
}

bool MercurialControl::managesDirectory(const QString &directory) const
{
    QFileInfo dir(directory);
    return !mercurialClient->findTopLevelForFile(dir).isEmpty();
}

QString MercurialControl::findTopLevelForDirectory(const QString &directory) const
{
    QFileInfo dir(directory);
    return mercurialClient->findTopLevelForFile(dir);
}

bool MercurialControl::supportsOperation(Operation operation) const
{
    bool supported = true;

    switch (operation) {
    case Core::IVersionControl::AddOperation:
    case Core::IVersionControl::DeleteOperation:
        break;
    case Core::IVersionControl::OpenOperation:
    default:
        supported = false;
        break;
    }

    return supported;
}

bool MercurialControl::vcsOpen(const QString &filename)
{
    Q_UNUSED(filename)
    return true;
}

bool MercurialControl::vcsAdd(const QString &filename)
{
    return mercurialClient->add(filename);
}

bool MercurialControl::vcsDelete(const QString &filename)
{
    return mercurialClient->remove(filename);
}

bool MercurialControl::sccManaged(const QString &filename)
{
    return mercurialClient->manifestSync(filename);
}
