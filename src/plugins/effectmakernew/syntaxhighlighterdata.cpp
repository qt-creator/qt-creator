// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "syntaxhighlighterdata.h"

namespace EffectMaker {

static constexpr QByteArrayView shader_arg_names[] {
    { "gl_Position" },
    { "qt_MultiTexCoord0" },
    { "qt_Vertex" },
    { "qt_Matrix" },
    { "qt_Opacity" },
    { "vertCoord" },
    { "fragCoord" },
    { "texCoord" },
    { "fragColor" },
    { "iMouse" },
    { "iResolution" },
    { "iTime" },
    { "iFrame" },
    { "iSource" },
    { "iSourceBlur1" },
    { "iSourceBlur2" },
    { "iSourceBlur3" },
    { "iSourceBlur4" },
    { "iSourceBlur5" },
    { "iSourceBlur6" }
};

static constexpr QByteArrayView shader_tag_names[] {
    { "@main" },
    { "@nodes" },
    { "@mesh" },
    { "@blursources" },
    { "@requires" }
};

// From https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.40.pdf
// Not including functions only available with compatibility profile
static constexpr QByteArrayView shader_function_names[] {
    { "radians()" },
    { "degrees()" },
    { "sin()" },
    { "cos()" },
    { "tan()" },
    { "asin()" },
    { "acos()" },
    { "atan()" },
    { "sinh()" },
    { "cosh()" },
    { "tanh()" },
    { "asinh()" },
    { "acosh()" },
    { "atanh()" },
    { "pow()" },
    { "exp()" },
    { "log()" },
    { "exp2()" },
    { "log2()" },
    { "sqrt()" },
    { "inversesqrt()" },
    { "abs()" },
    { "sign()" },
    { "floor()" },
    { "trunc()" },
    { "round()" },
    { "roundEven()" },
    { "ceil()" },
    { "fract()" },
    { "mod()" },
    { "modf()" },
    { "min()" },
    { "max()" },
    { "clamp()" },
    { "mix()" },
    { "step()" },
    { "smoothstep()" },
    { "isnan()" },
    { "isinf()" },
    { "floatBitsToInt()" },
    { "intBitsToFloat()" },
    { "fma()" },
    { "frexp()" },
    { "ldexp()" },
    { "packUnorm2x16()" },
    { "packSnorm2x16()" },
    { "packUnorm4x8()" },
    { "packSnorm4x8()" },
    { "unpackUnorm2x16()" },
    { "unpackSnorm2x16()" },
    { "unpackUnorm4x8()" },
    { "unpackSnorm4x8()" },
    //{ "packDouble2x32()" }, // Not supported in HLSL
    //{ "unpackDouble2x32()" },
    { "packHalf2x16()" },
    { "unpackHalf2x16()" },
    { "length()" },
    { "distance()" },
    { "dot()" },
    { "cross()" },
    { "normalize()" },
    { "faceforward()" },
    { "reflect()" },
    { "refract()" },
    { "matrixCompMult()" },
    { "outerProduct()" },
    { "transpose()" },
    { "determinant()" },
    { "inverse()" },
    { "lessThan()" },
    { "lessThanEqual()" },
    { "greaterThan()" },
    { "greaterThanEqual()" },
    { "equal()" },
    { "notEqual()" },
    { "any()" },
    { "all()" },
    { "not()" },
    //{ "uaddCarry()" }, // Extended arithmetic is only available from ESSL 310
    //{ "usubBorrow()" },
    //{ "umulExtended()" },
    //{ "imulExtended()" },
    { "bitfieldExtract()" },
    { "bitfieldInsert()" },
    { "bitfieldReverse()" },
    { "bitCount()" },
    { "findLSB()" },
    { "findMSB()" },
    { "textureSize()" },
    //{ "textureQueryLod()" }, // ImageQueryLod is only supported on MSL 2.2 and up.
    //{ "textureQueryLevels()" }, // textureQueryLevels not supported in ES profile.
    { "texture()" },
    { "textureProj()" },
    { "textureLod()" },
    { "textureOffset()" },
    { "texelFetch()" },
    { "texelFetchOffset()" },
    { "textureProjOffset()" },
    { "textureLodOffset()" },
    { "textureProjLod()" },
    { "textureProjLodOffset()" },
    { "textureGrad()" },
    { "textureGradOffset()" },
    { "textureProjGrad()" },
    { "textureProjGradOffset()" },
    //{ "textureGather()" }, // textureGather requires ESSL 310.
    //{ "textureGatherOffset()" },
    //{ "textureGatherOffsets()" },
    //{ "atomicCounterIncrement()" }, // 'atomic counter types' : not allowed when using GLSL for Vulkan
    //{ "atomicCounterDecrement()" },
    //{ "atomicCounter()" },
    //{ "atomicAdd()" }, // HLSL: interlocked targets must be groupshared or UAV elements
    //{ "atomicMin()" },
    //{ "atomicMax()" },
    //{ "atomicAnd()" },
    //{ "atomicOr()" },
    //{ "atomicXor()" },
    //{ "atomicExchange()" },
    //{ "atomicCompSwap()" },
    { "dFdx()" },
    { "dFdy()" },
    { "fwidth()" }
    //{ "interpolateAtCentroid()" }, // Pull-model interpolation requires MSL 2.3.
    //{ "interpolateAtSample()" },
    //{ "interpolateAtOffset()" }
};

SyntaxHighlighterData::SyntaxHighlighterData()
{
}


QList<QByteArrayView> SyntaxHighlighterData::reservedArgumentNames()
{
    return { std::begin(shader_arg_names), std::end(shader_arg_names) };
}

QList<QByteArrayView> SyntaxHighlighterData::reservedTagNames()
{
    return { std::begin(shader_tag_names), std::end(shader_tag_names) };
}

QList<QByteArrayView> SyntaxHighlighterData::reservedFunctionNames()
{
    return { std::begin(shader_function_names), std::end(shader_function_names) };
}

} // namespace EffectMaker


