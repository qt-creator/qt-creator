#include "invalidmetainfoexception.h"

/*!
\class QKineticDesigner::InvalidMetaInfoException
\ingroup CoreExceptions
\brief Exception for a invalid meta info

\see NodeMetaInfo PropertyMetaInfo MetaInfo
*/
namespace QKineticDesigner {
/*!
\brief Constructor

\param line use the __LINE__ macro
\param function use the __FUNCTION__ or the Q_FUNC_INFO macro
\param file use the __FILE__ macro
*/
InvalidMetaInfoException::InvalidMetaInfoException(int line,
                                                           const QString &function,
                                                           const QString &file)
 : Exception(line, function, file)
{
}

/*!
\brief Returns the type of this exception

\returns the type as a string
*/
QString InvalidMetaInfoException::type() const
{
    return "InvalidMetaInfoException";
}

}
