/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickContainers library.
//
// \file    qcmContainerConcept.h
// \author  benoit@destrat.io
// \date    2016 11 28
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QList>
#include <QVector>
#include <QSet>

// STD headers
#include <memory>       // shared_ptr, weak_ptr
#include <vector>
#include <type_traits>  // integral_constant
#include <algorithm>    // std::remove

namespace qcm { // ::qcm

/*! \brief
 *
 * \warning To keep compatibility with Qt, adapter<>::indexOf() return an int instead of a
 * size_t which is the default behaviour for STL.
 *
 * \warning Return value interpretation differ between Qt and Std lib for method removeAll(): with
 * Qt containers (QList, QVector, QSet) the number of removed is returned, for Std lib (std::vector, etc.)
 * it _always_ return -1.
 */
template <template<typename...CArgs> class Container, typename T >
struct adapter { };

template <typename T>
struct adapter<QList, T> {
    inline static void  reserve(QList<T>& c, std::size_t size) { c.reserve(static_cast<int>(size)); }

    inline static void  append(QList<T>& c, const T& t) { c.append(t); }
    inline static void  append(QList<T>& c, T&& t)      { c.append(t); }

    inline static void  insert(QList<T>& c, const T& t) { c.append(t); }
    inline static void  insert(QList<T>& c, T&& t)      { c.append(t); }
    inline static void  insert(QList<T>& c, const T& t, std::size_t i ) { c.insert(i, t); }

    inline static void  remove(QList<T>& c, std::size_t i) { c.removeAt(static_cast<int>(i)); }
    inline static int   removeAll(QList<T>& c, const T& t) { return c.removeAll(t); }

    inline static bool  contains(const QList<T>& c, const T& t) { return c.contains(t); }
    inline static int   indexOf(const QList<T>& c, const T& t) { return c.indexOf(t); }
};

#if (defined(__clang__) || defined(_MSC_VER))
template <typename T>
struct adapter<QVector, T> {
    inline static void  reserve(QVector<T>& c, std::size_t size) { c.reserve(static_cast<int>(size)); }

    inline static void  append(QVector<T>& c, const T& t)   { c.append(t); }
    inline static void  append(QVector<T>& c, T&& t)        { c.append(t); }

    inline static void  insert(QVector<T>& c, const T& t)   { c.append(t); }
    inline static void  insert(QVector<T>& c, T&& t)        { c.append(t); }
    inline static void  insert(QVector<T>& c, const T& t, int i ) { c.insert(i, t); }

    inline static void  remove(QVector<T>& c, std::size_t i)    { c.remove(static_cast<int>(i)); }
    inline static int   removeAll(QVector<T>& c, const T& t )   { return c.removeAll(t); }

    inline static bool  contains(const QVector<T>& c, const T& t) { return c.contains(t); }
    inline static int   indexOf(const QVector<T>& c, const T& t) { return c.indexOf(t); }
};
#endif

template <typename T>
struct adapter<QSet, T> {
    inline static void  reserve(QSet<T>& c, std::size_t size) { c.reserve(static_cast<int>(size)); }

    inline static void  append(QSet<T>& c, const T& t)  { c.insert(t); }
    inline static void  append(QSet<T>& c, T&& t)       { c.insert(t); }

    inline static void  insert(QSet<T>& c, const T& t)      { c.insert(t); }
    inline static void  insert(QSet<T>& c, T&& t)           { c.insert(t); }
    inline static void  insert(QSet<T>& c, const T& t, int i )    { c.insert(t); Q_UNUSED(i); }

    inline static void  remove(QSet<T>& c, std::size_t i)   { c.erase(c.cbegin() + static_cast<int>(i)); }
    inline static int   removeAll(QSet<T>& c, const T& t )  { return c.remove(t); }

    inline static bool  contains(const QSet<T>& c, const T& t) { return c.contains(t); }
    inline static int   indexOf(const QSet<T>& c, const T& t) {
        const auto r = c.find(t);
        return r == c.cend() ? -1 : std::distance(c.cbegin(), r);
    }
};

template <typename T>
struct adapter<std::vector, T> {
    inline static void  reserve(std::vector<T>& c, std::size_t size) { c.reserve(size); }

    inline static void  append(std::vector<T>& c, const T& t)   { c.push_back(t); }
    inline static void  append(std::vector<T>& c, T&& t)        { c.push_back(t); }

    inline static void  insert(std::vector<T>& c, const T& t)   { c.push_back(t); }
    inline static void  insert(std::vector<T>& c, T&& t)        { c.push_back(t); }
    inline static void  insert(std::vector<T>& c, const T& t, std::size_t i) { c.insert(c.begin() + i, t); }

    inline static void  remove(std::vector<T>& c, std::size_t i) { c.erase(c.cbegin() + i); }
    // See erase-remove idiom:
    // http://thispointer.com/removing-all-occurences-of-an-element-from-vector-in-on-complexity/
    inline static int   removeAll(std::vector<T>& c, const T& t) {
        c.erase(std::remove(c.begin(), c.end(), t), c.end());
        return -1;
    }

    inline static bool  contains(const std::vector<T>& c, const T& t) {
        const auto r = std::find_if(c.cbegin(), c.cend(), [t](const auto& a) noexcept { return t == a; } );
        return r != c.cend();
    }
    inline static int   indexOf(const std::vector<T>& c, const T& t) {
        const auto r = std::find_if(c.cbegin(), c.cend(), [t](const auto& a) noexcept { return t == a; } );
        return r == c.cend() ? -1 : std::distance(c.cbegin(), r);
    }
};

} // ::qcm
