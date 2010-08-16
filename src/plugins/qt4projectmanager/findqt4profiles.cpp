#include "findqt4profiles.h"
#include "qt4nodes.h"

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

QList<Qt4ProFileNode *> FindQt4ProFiles::operator()(ProjectExplorer::ProjectNode *root)
{
    m_proFiles.clear();
    root->accept(this);
    return m_proFiles;
}

void FindQt4ProFiles::visitProjectNode(ProjectExplorer::ProjectNode *projectNode)
{
    if (Qt4ProFileNode *pro = qobject_cast<Qt4ProFileNode *>(projectNode))
        m_proFiles.append(pro);
}
