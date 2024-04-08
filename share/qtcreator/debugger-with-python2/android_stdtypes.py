# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


from dumper import DumperBase
from utils import DisplayFormat

import stdtypes
import libcpp_stdtypes


def qform__std____ndk1__array():
    return [DisplayFormat.ArrayPlot]


def qdump__std____ndk1__array(d, value):
    stdtypes.qdump__std__array(d, value)


def qdump__std____ndk1__complex(d, value):
    stdtypes.qdump__std__complex(d, value)


def qdump__std____ndk1__deque(d, value):
    stdtypes.qdumpHelper__std__deque__libcxx(d, value)


def qdump__std____ndk1__list(d, value):
    return libcpp_stdtypes.qdump__std____1__list(d, value)


def qdump__std____ndk1__set(d, value):
    return libcpp_stdtypes.qdump__std____1__set(d, value)


def qdump__std____ndk1__multiset(d, value):
    qdump__std____ndk1__set(d, value)


def qform__std____ndk1__map():
    return [DisplayFormat.CompactMap]


def qdump__std____ndk1__map(d, value):
    return libcpp_stdtypes.qdump__std____1__map(d, value)


def qform__std____ndk1__multimap():
    return [DisplayFormat.CompactMap]


def qdump__std____ndk1__multimap(d, value):
    qdump__std____ndk1__map(d, value)


def qdump__std____ndk1__map__iterator(d, value):
    return libcpp_stdtypes.qdump__std____1__map__iterator(d, value)


def qdump__std____ndk1__map__const_iterator(d, value):
    qdump__std____ndk1__map__iterator(d, value)


def qdump__std____ndk1__set__iterator(d, value):
    return libcpp_stdtypes.qdump__std____1__set__iterator(d, value)


def qdump__std____ndk1__set_const_iterator(d, value):
    qdump__std____ndk1__set__iterator(d, value)


def qdump__std____ndk1__stack(d, value):
    return libcpp_stdtypes.qdump__std____1__stack(d, value)


def qdump__std____ndk1__string(d, value):
    return libcpp_stdtypes.qdump__std____1__string(d, value)


def qdump__std____ndk1__wstring(d, value):
    return libcpp_stdtypes.qdump__std____1__wstring(d, value)


def qdump__std____ndk1__basic_string(d, value):
    return libcpp_stdtypes.qdump__std____1__basic_string(d, value)


def qdump__std____ndk1__shared_ptr(d, value):
    return libcpp_stdtypes.qdump__std____1__shared_ptr(d, value)


def qdump__std____ndk1__weak_ptr(d, value):
    return qdump__std____ndk1__shared_ptr(d, value)


def qdump__std____ndk1__unique_ptr(d, value):
    stdtypes.qdump__std__unique_ptr(d, value)


def qform__std____ndk1__unordered_map():
    return [DisplayFormat.CompactMap]


def qdump__std____ndk1__unordered_map(d, value):
    libcpp_stdtypes.qdump__std____1__unordered_map(d, value)


def qdump__std____ndk1__unordered_set(d, value):
    return libcpp_stdtypes.qdump__std____1__unordered_set(d, value)


def qdump__std____ndk1__unordered_multiset(d, value):
    qdump__std____ndk1__unordered_set(d, value)


def qform__std____ndk1__valarray():
    return [DisplayFormat.ArrayPlot]


def qdump__std____ndk1__valarray(d, value):
    return libcpp_stdtypes.qdump__std____1__valarray(d, value)


def qform__std____ndk1__vector():
    return [DisplayFormat.ArrayPlot]


def qdump__std____ndk1__vector(d, value):
    stdtypes.qdumpHelper__std__vector__libcxx(d, value)


def qdump__std____ndk1__once_flag(d, value):
    stdtypes.qdump__std__once_flag(d, value)
