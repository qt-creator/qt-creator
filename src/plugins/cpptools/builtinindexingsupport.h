#ifndef BUILTININDEXINGSUPPORT_H
#define BUILTININDEXINGSUPPORT_H

#include "cppindexingsupport.h"
#include "ModelManagerInterface.h"

#include <QFutureSynchronizer>

namespace CppTools {
namespace Internal {

class BuiltinIndexingSupport: public CppIndexingSupport {
public:
    typedef CPlusPlus::CppModelManagerInterface::WorkingCopy WorkingCopy;

public:
    BuiltinIndexingSupport();
    ~BuiltinIndexingSupport();

    virtual QFuture<void> refreshSourceFiles(const QStringList &sourceFiles);
    virtual SymbolSearcher *createSymbolSearcher(SymbolSearcher::Parameters parameters, QSet<QString> fileNames);

private:
    QFutureSynchronizer<void> m_synchronizer;
    unsigned m_revision;
    bool m_dumpFileNameWhileParsing;
};

} // namespace Internal
} // namespace CppTools

#endif // BUILTININDEXINGSUPPORT_H
