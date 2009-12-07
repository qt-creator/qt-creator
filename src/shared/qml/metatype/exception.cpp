#include "exception.h"

#ifdef Q_OS_LINUX
#include <execinfo.h>
#include <cxxabi.h>
#endif

#include <QRegExp>

/*!
\defgroup CoreExceptions
*/
/*!
\class QKineticDesigner::Exception
\ingroup CoreExceptions
\brief This is the abstract base class for all excetions.
    Exceptions should be used in cases there is no other way to say something goes wrong. For example
    the result would be a inconsistent model or a crash.
*/


namespace QKineticDesigner {

#ifdef Q_OS_LINUX
const char* demangle(const char* name)
{
   char buf[1024];
   size_t size = 1024;
   int status;
   char* res;
   res = abi::__cxa_demangle(name,
     buf,
     &size,
     &status);
   return res;
}
#else
const char* demangle(const char* name)
{
   return name;
}
#endif


bool Exception::s_shouldAssert = true;

void Exception::setShouldAssert(bool assert)
{
    s_shouldAssert = assert;
}

bool Exception::shouldAssert()
{
    return s_shouldAssert;
}

/*!
\brief Constructor

\param line use the __LINE__ macro
\param function use the __FUNCTION__ or the Q_FUNC_INFO macro
\param file use the __FILE__ macro
*/
Exception::Exception(int line,
              const QString &function,
              const QString &file)
  : m_line(line),
    m_function(function),
    m_file(file)
{
#ifdef Q_OS_LINUX
    void * array[50];
    int nSize = backtrace(array, 50);
    char ** symbols = backtrace_symbols(array, nSize);

    for (int i = 0; i < nSize; i++)
    {
        m_backTrace.append(QString("%1\n").arg(symbols[i]));
    }

    free(symbols);
#endif

if (s_shouldAssert)
    Q_ASSERT_X(false, function.toLatin1(), QString("%1:%2 - %3").arg(file).arg(line).arg(function).toLatin1());
}

Exception::~Exception()
{
}

/*!
\brief Returns the unmangled backtrace of this exception

\returns the backtrace as a string
*/
QString Exception::backTrace() const
{
    return m_backTrace;
}

/*!
\brief Returns the optional description of this exception

\returns the description as string
*/
QString Exception::description() const
{
    return QString();
}

/*!
\brief Returns the line number where this exception was thrown

\returns the line number as integer
*/
int Exception::line() const
{
    return m_line;
}

/*!
\brief Returns the function name where this exception was thrown

\returns the function name as string
*/
QString Exception::function() const
{
    return m_function;
}

/*!
\brief Returns the file name where this exception was thrown

\returns the file name as string
*/
QString Exception::file() const
{
    return m_file;
}

QDebug operator<<(QDebug debug, const Exception &exception)
{
    debug.nospace() << "Exception: " << exception.type() << "\n"
                       "Function:  " << exception.function() << "\n"
                       "File:      " << exception.file() << "\n"
                       "Line:      " << exception.line() << "\n";
    if (!exception.description().isEmpty())
        debug.nospace() << exception.description();

    if (!exception.backTrace().isEmpty())
        debug.nospace() << exception.backTrace();

    return debug.space();
}

/*!
\fn QString Exception::type() const
\brief Returns the type of this exception

\returns the type as a string
*/
}
