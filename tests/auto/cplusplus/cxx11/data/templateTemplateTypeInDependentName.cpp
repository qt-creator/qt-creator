template<template<typename> class T> struct S {};
template<typename T> using __S = typename S<T::template __type>::type;
