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

#pragma once

//
// The following "non-latin1" code points are used in the tests:
//
//   U+00FC  - 2 code units in UTF8, 1 in UTF16 - LATIN SMALL LETTER U WITH DIAERESIS
//   U+4E8C  - 3 code units in UTF8, 1 in UTF16 - CJK UNIFIED IDEOGRAPH-4E8C
//   U+10302 - 4 code units in UTF8, 2 in UTF16 - OLD ITALIC LETTER KE
//

#define UC_U00FC "\xc3\xbc"
#define UC_U4E8C "\xe4\xba\x8c"
#define UC_U10302 "\xf0\x90\x8c\x82"
#define TEST_UNICODE_IDENTIFIER UC_U00FC UC_U4E8C UC_U10302
