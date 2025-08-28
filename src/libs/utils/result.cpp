// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "result.h"

#include "utilstr.h"

/*!
    \class Utils::Result
    \inmodule QtCreator

    \brief Result<T> is used for returning either a success value or an error string
    from a function.

    The Result typedef is a shorthand for \c {expected<T, QString>}, with the \e expected
    value of type \c T being returned in the success case, and the \e unexpected value being
    an error message for the user.

    Use \c Result<> as the type when you do not have a concrete success value to
    return (do \b not use \c {Result<bool>}). In that case, return Utils::ResultOk to return
    \e success. You can also use the convenience function Utils::makeResult to combine
    the success and error cases.

    Return a Utils::ResultError value as a convenience class for the error case.

    \sa Utils::ResultOk
    \sa Utils::ResultError
    \sa Utils::makeResult
*/

/*!
    \variable Utils::ResultOk

    Use the global object ResultOk to return \e success of type \c {Result<>}.

    \sa Utils::Result
*/

/*!
    \class Utils::ResultError
    \inmodule QtCreator

    \brief The ResultError class is used for returning an error including an error message for the
    user from a function.

    In addition to a convenience constructor with an error message string,
    the class provides standardized error messages for internal errors via the
    Utils::ResultSpecialErrorCode enum.

    \sa Utils::Result
    \sa Utils::ResultSpecialErrorCode
*/

/*!
    \enum Utils::ResultSpecialErrorCode

    The ResultSpecialErrorCode enum is used with the Utils::ResultError class
    to provide a set of standardized error messages.

    \value ResultAssert
           An internal error that indicates that preconditions are violated.
    \value ResultUnimplemented
           A backend unexpectedly did not provide an implementation of this functionality.

    \sa Utils::ResultError
*/

namespace Utils {

const Result<> ResultOk;

static QString messageForCode(ResultSpecialErrorCode code)
{
    switch (code) {
    case ResultAssert:
        return Tr::tr("Internal error: %1.");
    case ResultUnimplemented:
        return Tr::tr("Not implemented error: %1.");
    default:
        return Tr::tr("Unknown error: %1.");
    }
}

/*!
    Creates an error with the given \a errorMessage for the user.
*/
ResultError::ResultError(const QString &errorMessage)
    : m_error(errorMessage)
{}

/*!
    Creates an error with a standardized error message. Use \a errorMessage to provide
    further details on the error.

    \sa Utils::ResultSpecialErrorCode
*/
ResultError::ResultError(ResultSpecialErrorCode code, const QString &errorMessage)
    : m_error(messageForCode(code).arg(
          errorMessage.isEmpty() ? Tr::tr("Unknown reason.") : errorMessage))
{}

/*!
    Returns a result object that reports \e success if \a ok is true, or \e error with an
    \a errorMessage for the user if \a ok is false.

    \sa Utils::Result
*/
Result<> makeResult(bool ok, const QString &errorMessage)
{
    if (ok)
        return ResultOk;
    return ResultError(errorMessage);
}

} // Utils
