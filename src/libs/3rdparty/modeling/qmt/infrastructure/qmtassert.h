#ifndef QMT_QMTASSERT_H
#define QMT_QMTASSERT_H

#include "utils/qtcassert.h"

// TODO extend with parameter action
#define QMT_CHECK(condition) QTC_CHECK(condition)
// TODO implement logging of where and what
#define QMT_CHECK_X(condition, where, what) QTC_CHECK(condition)

#endif // QMT_QMTASSERT_H
