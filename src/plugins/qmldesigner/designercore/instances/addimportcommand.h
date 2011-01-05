#ifndef ADDIMPORTCOMMAND_H
#define ADDIMPORTCOMMAND_H

#include "addimportcontainer.h"

namespace QmlDesigner {

class AddImportCommand
{
    friend QDataStream &operator>>(QDataStream &in, AddImportCommand &command);
public:
    AddImportCommand();
    AddImportCommand(const AddImportContainer &container);

    AddImportContainer import() const;

private:
    AddImportContainer m_importContainer;
};

QDataStream &operator<<(QDataStream &out, const AddImportCommand &command);
QDataStream &operator>>(QDataStream &in, AddImportCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::AddImportCommand)

#endif // ADDIMPORTCOMMAND_H
