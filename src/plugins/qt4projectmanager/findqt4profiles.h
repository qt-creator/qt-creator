#ifndef FINDQT4PROFILES_H
#define FINDQT4PROFILES_H

#include <projectexplorer/nodesvisitor.h>

namespace Qt4ProjectManager {
namespace Internal {

class Qt4ProFileNode;

class FindQt4ProFiles: protected ProjectExplorer::NodesVisitor {

public:
    QList<Qt4ProFileNode *> operator()(ProjectExplorer::ProjectNode *root);
protected:
    virtual void visitProjectNode(ProjectExplorer::ProjectNode *projectNode);
private:
    QList<Qt4ProFileNode *> m_proFiles;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // FINDQT4PROFILES_H

