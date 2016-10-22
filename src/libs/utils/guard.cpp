/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "guard.h"
#include "qtcassert.h"

/*! \class Utils::Guard

  \brief The Guard class implements a recursive guard with locking mechanism.

  It may be used as an alternative to QSignalBlocker.
  QSignalBlocker blocks all signals of the object
  which is usually not desirable. It may also block signals
  which are needed internally by the object itself.
  The Guard and GuardLocker classes don't block signals at all.

  When calling a object's method which may in turn emit a signal
  which you are connected to, and you want to ignore
  this notification, you should keep the Guard object
  as your class member and declare the GuardLocker object
  just before calling the mentioned method, like:

  \code
  class MyClass : public QObject
  {
  \dots
  private:
      Guard updateGuard; // member of your class
  };

  \dots

  void MyClass::updateOtherObject()
  {
      GuardLocker updatelocker(updateGuard);
      otherObject->update(); // this may trigger a signal
  }
  \endcode

  Inside a slot which is connected to the other's object signal
  you may check if the guard is locked and ignore the further
  operations in this case:

  \code
  void MyClass::otherObjectUpdated()
  {
      if (updateGuard.isLocked())
          return;

      // we didn't trigger the update
      // so do update now
      \dots
  }
  \endcode

  The GuardLocker unlocks the Guard in its destructor.

  The Guard object is recursive, you may declare many GuardLocker
  objects for the same Guard instance and the Guard will be locked
  as long as at least one GuardLocker object created for the Guard
  is in scope.
*/

namespace Utils {

Guard::Guard()
{
}

Guard::~Guard()
{
    QTC_CHECK(m_lockCount == 0);
}

bool Guard::isLocked() const
{
    return m_lockCount;
}

GuardLocker::GuardLocker(Guard &guard)
    : m_guard(guard)
{
    ++m_guard.m_lockCount;
}

GuardLocker::~GuardLocker()
{
    --m_guard.m_lockCount;
}

} // namespace Utils
