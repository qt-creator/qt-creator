/*
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "ansihighlighter.h"
#include "context_p.h"
#include "definition.h"
#include "definition_p.h"
#include "format.h"
#include "ksyntaxhighlighting_logging.h"
#include "state.h"
#include "state_p.h"
#include "theme.h"

#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextStream>

#include <cmath>
#include <vector>

using namespace KSyntaxHighlighting;

namespace
{
struct Lab {
    double L;
    double a;
    double b;
};

// clang-format off
    // xterm color reference
    // constexpr Rgb888 xterm256Colors[] {
    //     {0x00, 0x00, 0x00}, {0x80, 0x00, 0x00}, {0x00, 0x80, 0x00}, {0x80, 0x80, 0x00},
    //     {0x00, 0x00, 0x80}, {0x80, 0x00, 0x80}, {0x00, 0x80, 0x80}, {0xc0, 0xc0, 0xc0},
    //     {0x80, 0x80, 0x80}, {0xff, 0x00, 0x00}, {0x00, 0xff, 0x00}, {0xff, 0xff, 0x00},
    //     {0x00, 0x00, 0xff}, {0xff, 0x00, 0xff}, {0x00, 0xff, 0xff}, {0xff, 0xff, 0xff},
    //     {0x00, 0x00, 0x00}, {0x00, 0x00, 0x5f}, {0x00, 0x00, 0x87}, {0x00, 0x00, 0xaf},
    //     {0x00, 0x00, 0xd7}, {0x00, 0x00, 0xff}, {0x00, 0x5f, 0x00}, {0x00, 0x5f, 0x5f},
    //     {0x00, 0x5f, 0x87}, {0x00, 0x5f, 0xaf}, {0x00, 0x5f, 0xd7}, {0x00, 0x5f, 0xff},
    //     {0x00, 0x87, 0x00}, {0x00, 0x87, 0x5f}, {0x00, 0x87, 0x87}, {0x00, 0x87, 0xaf},
    //     {0x00, 0x87, 0xd7}, {0x00, 0x87, 0xff}, {0x00, 0xaf, 0x00}, {0x00, 0xaf, 0x5f},
    //     {0x00, 0xaf, 0x87}, {0x00, 0xaf, 0xaf}, {0x00, 0xaf, 0xd7}, {0x00, 0xaf, 0xff},
    //     {0x00, 0xd7, 0x00}, {0x00, 0xd7, 0x5f}, {0x00, 0xd7, 0x87}, {0x00, 0xd7, 0xaf},
    //     {0x00, 0xd7, 0xd7}, {0x00, 0xd7, 0xff}, {0x00, 0xff, 0x00}, {0x00, 0xff, 0x5f},
    //     {0x00, 0xff, 0x87}, {0x00, 0xff, 0xaf}, {0x00, 0xff, 0xd7}, {0x00, 0xff, 0xff},
    //     {0x5f, 0x00, 0x00}, {0x5f, 0x00, 0x5f}, {0x5f, 0x00, 0x87}, {0x5f, 0x00, 0xaf},
    //     {0x5f, 0x00, 0xd7}, {0x5f, 0x00, 0xff}, {0x5f, 0x5f, 0x00}, {0x5f, 0x5f, 0x5f},
    //     {0x5f, 0x5f, 0x87}, {0x5f, 0x5f, 0xaf}, {0x5f, 0x5f, 0xd7}, {0x5f, 0x5f, 0xff},
    //     {0x5f, 0x87, 0x00}, {0x5f, 0x87, 0x5f}, {0x5f, 0x87, 0x87}, {0x5f, 0x87, 0xaf},
    //     {0x5f, 0x87, 0xd7}, {0x5f, 0x87, 0xff}, {0x5f, 0xaf, 0x00}, {0x5f, 0xaf, 0x5f},
    //     {0x5f, 0xaf, 0x87}, {0x5f, 0xaf, 0xaf}, {0x5f, 0xaf, 0xd7}, {0x5f, 0xaf, 0xff},
    //     {0x5f, 0xd7, 0x00}, {0x5f, 0xd7, 0x5f}, {0x5f, 0xd7, 0x87}, {0x5f, 0xd7, 0xaf},
    //     {0x5f, 0xd7, 0xd7}, {0x5f, 0xd7, 0xff}, {0x5f, 0xff, 0x00}, {0x5f, 0xff, 0x5f},
    //     {0x5f, 0xff, 0x87}, {0x5f, 0xff, 0xaf}, {0x5f, 0xff, 0xd7}, {0x5f, 0xff, 0xff},
    //     {0x87, 0x00, 0x00}, {0x87, 0x00, 0x5f}, {0x87, 0x00, 0x87}, {0x87, 0x00, 0xaf},
    //     {0x87, 0x00, 0xd7}, {0x87, 0x00, 0xff}, {0x87, 0x5f, 0x00}, {0x87, 0x5f, 0x5f},
    //     {0x87, 0x5f, 0x87}, {0x87, 0x5f, 0xaf}, {0x87, 0x5f, 0xd7}, {0x87, 0x5f, 0xff},
    //     {0x87, 0x87, 0x00}, {0x87, 0x87, 0x5f}, {0x87, 0x87, 0x87}, {0x87, 0x87, 0xaf},
    //     {0x87, 0x87, 0xd7}, {0x87, 0x87, 0xff}, {0x87, 0xaf, 0x00}, {0x87, 0xaf, 0x5f},
    //     {0x87, 0xaf, 0x87}, {0x87, 0xaf, 0xaf}, {0x87, 0xaf, 0xd7}, {0x87, 0xaf, 0xff},
    //     {0x87, 0xd7, 0x00}, {0x87, 0xd7, 0x5f}, {0x87, 0xd7, 0x87}, {0x87, 0xd7, 0xaf},
    //     {0x87, 0xd7, 0xd7}, {0x87, 0xd7, 0xff}, {0x87, 0xff, 0x00}, {0x87, 0xff, 0x5f},
    //     {0x87, 0xff, 0x87}, {0x87, 0xff, 0xaf}, {0x87, 0xff, 0xd7}, {0x87, 0xff, 0xff},
    //     {0xaf, 0x00, 0x00}, {0xaf, 0x00, 0x5f}, {0xaf, 0x00, 0x87}, {0xaf, 0x00, 0xaf},
    //     {0xaf, 0x00, 0xd7}, {0xaf, 0x00, 0xff}, {0xaf, 0x5f, 0x00}, {0xaf, 0x5f, 0x5f},
    //     {0xaf, 0x5f, 0x87}, {0xaf, 0x5f, 0xaf}, {0xaf, 0x5f, 0xd7}, {0xaf, 0x5f, 0xff},
    //     {0xaf, 0x87, 0x00}, {0xaf, 0x87, 0x5f}, {0xaf, 0x87, 0x87}, {0xaf, 0x87, 0xaf},
    //     {0xaf, 0x87, 0xd7}, {0xaf, 0x87, 0xff}, {0xaf, 0xaf, 0x00}, {0xaf, 0xaf, 0x5f},
    //     {0xaf, 0xaf, 0x87}, {0xaf, 0xaf, 0xaf}, {0xaf, 0xaf, 0xd7}, {0xaf, 0xaf, 0xff},
    //     {0xaf, 0xd7, 0x00}, {0xaf, 0xd7, 0x5f}, {0xaf, 0xd7, 0x87}, {0xaf, 0xd7, 0xaf},
    //     {0xaf, 0xd7, 0xd7}, {0xaf, 0xd7, 0xff}, {0xaf, 0xff, 0x00}, {0xaf, 0xff, 0x5f},
    //     {0xaf, 0xff, 0x87}, {0xaf, 0xff, 0xaf}, {0xaf, 0xff, 0xd7}, {0xaf, 0xff, 0xff},
    //     {0xd7, 0x00, 0x00}, {0xd7, 0x00, 0x5f}, {0xd7, 0x00, 0x87}, {0xd7, 0x00, 0xaf},
    //     {0xd7, 0x00, 0xd7}, {0xd7, 0x00, 0xff}, {0xd7, 0x5f, 0x00}, {0xd7, 0x5f, 0x5f},
    //     {0xd7, 0x5f, 0x87}, {0xd7, 0x5f, 0xaf}, {0xd7, 0x5f, 0xd7}, {0xd7, 0x5f, 0xff},
    //     {0xd7, 0x87, 0x00}, {0xd7, 0x87, 0x5f}, {0xd7, 0x87, 0x87}, {0xd7, 0x87, 0xaf},
    //     {0xd7, 0x87, 0xd7}, {0xd7, 0x87, 0xff}, {0xd7, 0xaf, 0x00}, {0xd7, 0xaf, 0x5f},
    //     {0xd7, 0xaf, 0x87}, {0xd7, 0xaf, 0xaf}, {0xd7, 0xaf, 0xd7}, {0xd7, 0xaf, 0xff},
    //     {0xd7, 0xd7, 0x00}, {0xd7, 0xd7, 0x5f}, {0xd7, 0xd7, 0x87}, {0xd7, 0xd7, 0xaf},
    //     {0xd7, 0xd7, 0xd7}, {0xd7, 0xd7, 0xff}, {0xd7, 0xff, 0x00}, {0xd7, 0xff, 0x5f},
    //     {0xd7, 0xff, 0x87}, {0xd7, 0xff, 0xaf}, {0xd7, 0xff, 0xd7}, {0xd7, 0xff, 0xff},
    //     {0xff, 0x00, 0x00}, {0xff, 0x00, 0x5f}, {0xff, 0x00, 0x87}, {0xff, 0x00, 0xaf},
    //     {0xff, 0x00, 0xd7}, {0xff, 0x00, 0xff}, {0xff, 0x5f, 0x00}, {0xff, 0x5f, 0x5f},
    //     {0xff, 0x5f, 0x87}, {0xff, 0x5f, 0xaf}, {0xff, 0x5f, 0xd7}, {0xff, 0x5f, 0xff},
    //     {0xff, 0x87, 0x00}, {0xff, 0x87, 0x5f}, {0xff, 0x87, 0x87}, {0xff, 0x87, 0xaf},
    //     {0xff, 0x87, 0xd7}, {0xff, 0x87, 0xff}, {0xff, 0xaf, 0x00}, {0xff, 0xaf, 0x5f},
    //     {0xff, 0xaf, 0x87}, {0xff, 0xaf, 0xaf}, {0xff, 0xaf, 0xd7}, {0xff, 0xaf, 0xff},
    //     {0xff, 0xd7, 0x00}, {0xff, 0xd7, 0x5f}, {0xff, 0xd7, 0x87}, {0xff, 0xd7, 0xaf},
    //     {0xff, 0xd7, 0xd7}, {0xff, 0xd7, 0xff}, {0xff, 0xff, 0x00}, {0xff, 0xff, 0x5f},
    //     {0xff, 0xff, 0x87}, {0xff, 0xff, 0xaf}, {0xff, 0xff, 0xd7}, {0xff, 0xff, 0xff},
    //     {0x08, 0x08, 0x08}, {0x12, 0x12, 0x12}, {0x1c, 0x1c, 0x1c}, {0x26, 0x26, 0x26},
    //     {0x30, 0x30, 0x30}, {0x3a, 0x3a, 0x3a}, {0x44, 0x44, 0x44}, {0x4e, 0x4e, 0x4e},
    //     {0x58, 0x58, 0x58}, {0x62, 0x62, 0x62}, {0x6c, 0x6c, 0x6c}, {0x76, 0x76, 0x76},
    //     {0x80, 0x80, 0x80}, {0x8a, 0x8a, 0x8a}, {0x94, 0x94, 0x94}, {0x9e, 0x9e, 0x9e},
    //     {0xa8, 0xa8, 0xa8}, {0xb2, 0xb2, 0xb2}, {0xbc, 0xbc, 0xbc}, {0xc6, 0xc6, 0xc6},
    //     {0xd0, 0xd0, 0xd0}, {0xda, 0xda, 0xda}, {0xe4, 0xe4, 0xe4}, {0xee, 0xee, 0xee},
    // };

    // xterm color represented in Oklab
    // see rgbToOklab()
    constexpr Lab xterm240_Oklabs[] {
        // ignore the first 16 colors as they are unpredictable (user configurable)
        // {0x0p+0, 0x0p+0, 0x0p+0},
        // {0x1.2d5e6bee2c4f6p+5,  0x1.af99c042ea40cp+3,  0x1.e2f6ba84d2d25p+2},
        // {0x1.9fcc4f3622914p+5, -0x1.c105bf1d2d218p+3,  0x1.586870aec30a4p+3},
        // {0x1.d089126c75579p+5, -0x1.12107f2e3119p+2,   0x1.7d020b82d4b9cp+3},
        // {0x1.b1ce15c4fcb51p+4, -0x1.f203762eb1242p+0, -0x1.2b150ae3c14bep+4},
        // {0x1.50bc446f31833p+5,  0x1.078a1150b431ap+4, -0x1.44d35b3de7eafp+3},
        // {0x1.b27d96eb8f471p+5, -0x1.1ee8867b0e065p+3, -0x1.2f2f18261c69ap+1},
        // {0x1.431e523cc2f4dp+6, -0x1.ad694c777b8p-11,  -0x1.c7c3ea0c32ep-8},
        // {0x1.dfe5855ae1528p+5, -0x1.3ee1ad40618p-11,  -0x1.5273b9784a3p-8},
        // {0x1.f663baac570efp+5,  0x1.67be9f690c994p+4,  0x1.928e76c3750aep+3},
        // {0x1.5a92b1ff8af32p+6, -0x1.76441609cfb3ap+4,  0x1.1f1186319beaap+4},
        // {0x1.83323c984ee7ap+6, -0x1.c8df58716d4cbp+2,  0x1.3d9335b5d20f9p+4},
        // {0x1.69950098864afp+5, -0x1.9f19c42a8c674p+1, -0x1.f293e325bec2ep+4},
        // {0x1.18ac5c68852cdp+6,  0x1.b753a9bd5dbeep+4, -0x1.0ebf0dff35bfbp+4},
        // {0x1.6a27499e4d3d6p+6, -0x1.de4892c062f8p+3,  -0x1.f96a59bc9de4cp+1},
        // {0x1.8ffffbb77c76ap+6, -0x1.09cab717214p-10,  -0x1.1a1aa7765a7p-7},
        // 240 colors mode
        {0x0p+0,                0x0p+0,                0x0p+0},
        {0x1.5f181b2779cap+4,  -0x1.930f78e22f09ap+0, -0x1.e41dbddfca08ap+3},
        {0x1.c2d3be821f882p+4, -0x1.02c70dd8af008p+1, -0x1.36d1623919ffp+4},
        {0x1.10a39beeb2926p+5, -0x1.38fe38b7dab01p+1, -0x1.77efa95d520b4p+4},
        {0x1.3de43fe8d92efp+5, -0x1.6cf188320fff2p+1, -0x1.b655790a27192p+4},
        {0x1.69950098864afp+5, -0x1.9f19c42a8c674p+1, -0x1.f293e325bec2ep+4},
        {0x1.50853f46a9f9ep+5, -0x1.6b6901a80404fp+3,  0x1.16bdec11e60d8p+3},
        {0x1.5fa625f2c3fbcp+5, -0x1.d0691ed5aa7ap+2,  -0x1.eac16e8cb9241p+0},
        {0x1.6f4222fc3f0dbp+5, -0x1.61b0e7ffa5e8ap+2, -0x1.05e8906ee23d7p+3},
        {0x1.83e99e6187f46p+5, -0x1.1a61c98b895p+2,   -0x1.c3da094bbcf2fp+3},
        {0x1.9ca47689a503dp+5, -0x1.e9259e3439104p+1, -0x1.396812c634de3p+4},
        {0x1.b872b55db6144p+5, -0x1.ccd50a2fd6662p+1, -0x1.89dec898ec996p+4},
        {0x1.b01d15d276ef1p+5, -0x1.d2a4448f6ccacp+3,  0x1.65ec167dcb488p+3},
        {0x1.b9760adf5c444p+5, -0x1.6a3760af4b6c4p+3,  0x1.a26bb322495e2p+1},
        {0x1.c38a22a31944p+5,  -0x1.2a2a93c4b741p+3,  -0x1.3b14a376ffbecp+1},
        {0x1.d185a36cfcd6ep+5, -0x1.e760f29e8fcb4p+2, -0x1.0bf936f2a6743p+3},
        {0x1.e321d754f2b44p+5, -0x1.95a749a95debp+2,  -0x1.c3928e31b9cf8p+3},
        {0x1.f7eb8d9ad84b9p+5, -0x1.5e28b7cc193ddp+2, -0x1.38bda7259441ep+4},
        {0x1.0552717cdb82p+6,  -0x1.1a33f75f67b0fp+4,  0x1.b0e8bd24c3fdep+3},
        {0x1.08898a5b7805dp+6, -0x1.e29bd3071d97ap+3,  0x1.e0b5994e0238ep+2},
        {0x1.0c10ad1b87866p+6, -0x1.a54f2215371a5p+3,  0x1.45758e5457898p+1},
        {0x1.1111e924447e9p+6, -0x1.68a24e1d5efe1p+3, -0x1.7d178a838ea32p+1},
        {0x1.178bb94d30a5cp+6, -0x1.335d7d75dd017p+3, -0x1.13f78c003ba13p+3},
        {0x1.1f6aa49fcff2p+6,  -0x1.0830d167759a4p+3, -0x1.c578e33f21d88p+3},
        {0x1.30b236b57ac86p+6, -0x1.490afbe3d8e11p+4,  0x1.f8c35fbb689dap+3},
        {0x1.331144aae0ad4p+6, -0x1.289bac3eeb83p+4,   0x1.626ea3a63748p+3},
        {0x1.35b09f5c62a7ap+6, -0x1.0d33e3db59803p+4,  0x1.b66836d6f7ff6p+2},
        {0x1.3973d5b39baa4p+6, -0x1.de87e61f0c86ap+3,  0x1.de8cf8346969cp+0},
        {0x1.3e64dbeab2c38p+6, -0x1.a47dea5d85dbbp+3, -0x1.bc586cdcba45p+1},
        {0x1.44815d7f73a41p+6, -0x1.706c35b4850ecp+3, -0x1.1d13e731dfc5bp+3},
        {0x1.5a92b1ff8af32p+6, -0x1.76441609cfb3ap+4,  0x1.1f1186319beaap+4},
        {0x1.5c6885a0ab4cap+6, -0x1.5c0bf4148d03dp+4,  0x1.c5b4a75d0a01ep+3},
        {0x1.5e72281eb918cp+6, -0x1.4422b7d2ad44bp+4,  0x1.5305a49da3492p+3},
        {0x1.61631d5752788p+6, -0x1.282b31110d79dp+4,  0x1.8b1a60b561753p+2},
        {0x1.65488920f4795p+6, -0x1.0b1a0cfacacfbp+4,  0x1.3d72a3bb176fp+0},
        {0x1.6a27499e4d3d6p+6, -0x1.de4892c062f8p+3,  -0x1.f96a59bc9de4cp+1},
        {0x1.e7d1475ebe201p+4,  0x1.5d4f5ebb6cf8ep+3,  0x1.86e150bac0e61p+2},
        {0x1.10883ee613f6bp+5,  0x1.aa957aceb4328p+3, -0x1.06e4a6bcf37ep+3},
        {0x1.2a507c82ee1e7p+5,  0x1.880d450b132c9p+3, -0x1.c81b5ba73664ap+3},
        {0x1.4902475f20191p+5,  0x1.4f4dfeda4e013p+3, -0x1.38d6c960a7255p+4},
        {0x1.6aa42ff68fb65p+5,  0x1.1116140836dp+3,   -0x1.84f2dbb4bf7b2p+4},
        {0x1.8def7d6adc3d5p+5,  0x1.ab6406f65a5b9p+2, -0x1.caee116cbc66ep+4},
        {0x1.77f724f99d0c9p+5, -0x1.bb9ee0e906bf2p+1,  0x1.345d15707c9e4p+3},
        {0x1.8465d178eda3bp+5, -0x1.021519c6d64p-11,  -0x1.11ebea130b3p-8},
        {0x1.917d476fba08dp+5,  0x1.84e9b4b9a4816p+0, -0x1.89910d2df7414p+2},
        {0x1.a32d016052029p+5,  0x1.352fbc1045df3p+1, -0x1.847eed4ab1e03p+3},
        {0x1.b8cfd7baeb72ap+5,  0x1.5d6575ced028ep+1, -0x1.1bff70d82f589p+4},
        {0x1.d1a20bfd91dbap+5,  0x1.4de285fe39a4dp+1, -0x1.6f381fca63bcep+4},
        {0x1.c99e943a2d79fp+5, -0x1.219ec15c5de22p+3,  0x1.78f2db7cd8205p+3},
        {0x1.d20af5c832c06p+5, -0x1.86c31353f2968p+2,  0x1.15f4d958ffb6ep+2},
        {0x1.db2d2b76b7db4p+5, -0x1.107aa185fa178p+2, -0x1.3a545f6bb32fep+0},
        {0x1.e7ef429658179p+5, -0x1.58a57459e2113p+1, -0x1.c50012145074dp+2},
        {0x1.f821bad822a9ep+5, -0x1.8ea06e2a9e1f6p+0, -0x1.9a5cf82ebb612p+3},
        {0x1.05b4f2f9696abp+6, -0x1.b3645fe7f9294p-1, -0x1.24f471b4e8b62p+4},
        {0x1.0e4a1f1b1ddcep+6, -0x1.b32bbdf7204c1p+3,  0x1.be3e8e0b04a1fp+3},
        {0x1.114fcf04f35b3p+6, -0x1.6696d5115c7e4p+3,  0x1.058704ea2708p+3},
        {0x1.14a2558b5b6c7p+6, -0x1.2c7e211429d81p+3,  0x1.a9f809f8ee4dfp+1},
        {0x1.195c00cf22a51p+6, -0x1.e58de99c40fb6p+2, -0x1.0e4f6479339a5p+1},
        {0x1.1f7e4eea78cb7p+6, -0x1.809e6530a25d5p+2, -0x1.ee2cfac349d5ep+2},
        {0x1.26f9f5da0f33bp+6, -0x1.2ffe0404fe1e4p+2, -0x1.a87af760f19c2p+3},
        {0x1.37635555b270cp+6, -0x1.17deced6d377ep+4,  0x1.0159387931185p+4},
        {0x1.39aa8b444a1c7p+6, -0x1.f1b4258b74e35p+3,  0x1.70a1d9ed49333p+3},
        {0x1.3c30003dc4bdap+6, -0x1.bcf061cd62971p+3,  0x1.d845b8f464806p+2},
        {0x1.3fcf0a7e4fdb8p+6, -0x1.8316f8fb04f1cp+3,  0x1.3be7d797ab219p+1},
        {0x1.4492530767fa4p+6, -0x1.4af657f86bc35p+3, -0x1.69ccecfbead6p+1},
        {0x1.4a78e7a0304fbp+6, -0x1.18be182ded159p+3, -0x1.07ac09ccdfff4p+3},
        {0x1.5fc9ac083946p+6,  -0x1.4f840d59cd9aep+4,  0x1.22ef620ec7775p+4},
        {0x1.6192b2ae205fp+6,  -0x1.361d4509435d5p+4,  0x1.cfe893676e62ep+3},
        {0x1.638e487d443e1p+6, -0x1.1edcf418396fcp+4,  0x1.5f105a3f7b33p+3},
        {0x1.666b5164f3799p+6, -0x1.03988ade0159ep+4,  0x1.a6a2e5baac692p+2},
        {0x1.6a3706fa48d42p+6, -0x1.ce6b98f424c54p+3,  0x1.b67fa4fafc2e8p+0},
        {0x1.6ef6b7860b53fp+6, -0x1.97ce09961f218p+3, -0x1.b926ed0fbe897p+1},
        {0x1.3931bb83cb32dp+5,  0x1.c0894426a198dp+3,  0x1.f5ea328bf4058p+2},
        {0x1.4b9e77eb58ebfp+5,  0x1.108cfd41d7919p+4, -0x1.0eaf04c8d3b35p+2},
        {0x1.5df2d7bacd40ap+5,  0x1.11e15f9b225acp+4, -0x1.51924b9c514f3p+3},
        {0x1.756bc7d79519bp+5,  0x1.03a63c750b36bp+4, -0x1.052fc194cf52p+4},
        {0x1.90b3e1d276c5dp+5,  0x1.d79fd75fb3811p+3, -0x1.58e6a7a1da1fcp+4},
        {0x1.aea5654d2d631p+5,  0x1.9f14550c88c57p+3, -0x1.a57e86563dbcap+4},
        {0x1.9b6948efcb1e8p+5,  0x1.dddaa142e7b3ap+0,  0x1.4f83ba256abcfp+3},
        {0x1.a5f9bfdb796c8p+5,  0x1.3de3647070a5bp+2,  0x1.b416b1e2a3817p+0},
        {0x1.b1407c7a80b46p+5,  0x1.9d8674f672f17p+2, -0x1.112d7835e0443p+2},
        {0x1.c0b6552db89b1p+5,  0x1.d8c2a5262b398p+2, -0x1.480f10f6d0b5fp+3},
        {0x1.d3edc2b5ca1f7p+5,  0x1.edbf49a0778e4p+2, -0x1.fe5ca96caae15p+3},
        {0x1.ea519f7b4bf8cp+5,  0x1.e35c8bfe38bf9p+2, -0x1.5483ed4b7e5ebp+4},
        {0x1.e2c36ca30962cp+5, -0x1.1cd1878e91233p+2,  0x1.8bf558e4fbde3p+3},
        {0x1.ea67769abd5c1p+5, -0x1.c053e5b6c3b66p+0,  0x1.59302161a2865p+2},
        {0x1.f2ba2dd022991p+5, -0x1.4b64e809e68p-11,  -0x1.5fbb8b338b8p-8},
        {0x1.fe6b844bbf20fp+5,  0x1.8040f07e39554p+0, -0x1.7196ffd7de002p+2},
        {0x1.06aeef460087dp+6,  0x1.4afa79e390244p+1, -0x1.705a7c8493135p+3},
        {0x1.0fa5554a5c955p+6,  0x1.9dea7336ee828p+1, -0x1.1089bb9ca046ep+4},
        {0x1.178e9051b853ep+6, -0x1.3b3b8e6c52f1cp+3,  0x1.cc268b9a4e157p+3},
        {0x1.1a65db5c4c0d1p+6, -0x1.e697a167b2b86p+2,  0x1.1b4d0d1ca381dp+3},
        {0x1.1d86bd983524p+6,  -0x1.77ccb1bcef226p+2,  0x1.08e1384d16e75p+2},
        {0x1.21fc7ad985402p+6, -0x1.08e3f099fb00fp+2, -0x1.363e6fbac8902p+0},
        {0x1.27cb1d6dd9434p+6, -0x1.4f7ef424b1025p+1, -0x1.b1af4c945ce98p+2},
        {0x1.2ee710ec206cfp+6, -0x1.6ac7f2787bda1p+0, -0x1.89e8c18db3849p+3},
        {0x1.3e7c86695f695p+6, -0x1.ceac064fd3a97p+3,  0x1.06a67a74ff4cp+4},
        {0x1.40ac0504d65f2p+6, -0x1.935bda5136fdbp+3,  0x1.7fa8ea5683dffp+3},
        {0x1.4317a9d652f14p+6, -0x1.608608dcc2607p+3,  0x1.fc21edd271f8bp+2},
        {0x1.46928cf49114ap+6, -0x1.2883a0410f74bp+3,  0x1.8d4b911a8102bp+1},
        {0x1.4b27f53f2d1c8p+6, -0x1.e40f8922e26b2p+2, -0x1.11ca0bf0acee9p+1},
        {0x1.50d83b6f7788bp+6, -0x1.82a0417f98419p+2, -0x1.e18432a6676edp+2},
        {0x1.6567db05a012cp+6, -0x1.27d55f8b64f16p+4,  0x1.271e9400a42e4p+4},
        {0x1.6723b3feab73ap+6, -0x1.0f412c5a09d1ap+4,  0x1.dae43f7f7eb6ep+3},
        {0x1.6910cf423acf1p+6, -0x1.f152d3d18c3b3p+3,  0x1.6c04c44876eb2p+3},
        {0x1.6bd948eca017fp+6, -0x1.bc29d2c53f57ap+3,  0x1.c44a28d107a62p+2},
        {0x1.6f8a69cf10b1p+6,  -0x1.84b224c6d040cp+3,  0x1.1c910c629db67p+1},
        {0x1.7429ebc4e406dp+6, -0x1.4f4a0b735c531p+3, -0x1.73a39097eadefp+1},
        {0x1.7acf7694f8c6p+5,   0x1.0f40ef4bed7e8p+4,  0x1.2f88d81b23f2ep+3},
        {0x1.87b52573912c8p+5,  0x1.3e9cadbae2c71p+4, -0x1.21c127942ffc2p-1},
        {0x1.95308092c646cp+5,  0x1.4bd8ec93cbe28p+4, -0x1.b1ad2fc6cd5ccp+2},
        {0x1.a743d71712a69p+5,  0x1.4b428f09e4704p+4, -0x1.984b766ca527bp+3},
        {0x1.bd35da1ada54ap+5,  0x1.3ee7b023f1a36p+4, -0x1.252f399063a3p+4},
        {0x1.d638af110e2bfp+5,  0x1.2a6369728426p+4,  -0x1.777a4238ecc9cp+4},
        {0x1.c5db1e678f93bp+5,  0x1.be23ef0d11b9fp+2,  0x1.706e52e6555aep+3},
        {0x1.ceab15b1e5e2cp+5,  0x1.376364145a451p+3,  0x1.d7fd2a954eedcp+1},
        {0x1.d8302a174d63dp+5,  0x1.67a05469bf7c1p+3, -0x1.0236bbe6ffddap+1},
        {0x1.e56c6a282c6fep+5,  0x1.8915eed9a5994p+3, -0x1.fb2d49a257381p+2},
        {0x1.f622a3ef65e7fp+5,  0x1.98638aed6156bp+3, -0x1.b57e66e36241ap+3},
        {0x1.04f5c4d9c25cfp+6,  0x1.9710ff57a7b7ap+3, -0x1.32004fe7bd5b5p+4},
        {0x1.018127d59826cp+6,  0x1.1e619caaa3a8fp-1,  0x1.a49bd0215a96dp+3},
        {0x1.04e57659b4cf6p+6,  0x1.8024718692a3cp+1,  0x1.addd1b874609p+2},
        {0x1.089bd543a0e63p+6,  0x1.2a7652607e02cp+2,  0x1.8d85013677bap+0},
        {0x1.0ddb424d04fadp+6,  0x1.889c9b8223651p+2, -0x1.05aa1bc86a9d4p+2},
        {0x1.149da8fbd8909p+6,  0x1.ce4542c130c31p+2, -0x1.3934aac98d184p+3},
        {0x1.1cca409e32c6dp+6,  0x1.f855300b25ba5p+2, -0x1.eac45e49f86bap+3},
        {0x1.23f3e081f0e6fp+6, -0x1.587d5d644e818p+2,  0x1.deea03a7e9e2p+3},
        {0x1.26935d2d5bb58p+6, -0x1.a6ab45ebb79ddp+1,  0x1.38265c4c6d156p+3},
        {0x1.29783d4927cb6p+6, -0x1.aa081e388f0b6p+0,  0x1.4de28afb99798p+2},
        {0x1.2d9b5abdd0df6p+6, -0x1.90d2c34c4c8p-11,  -0x1.a96c3913736p-8},
        {0x1.3303704c60227p+6,  0x1.7842d686da2p+0,   -0x1.6001098473213p+2},
        {0x1.39a911d79bf6cp+6,  0x1.51ae39272f6fp+1,  -0x1.604eff9e13224p+3},
        {0x1.483b66bb75f41p+6, -0x1.53e4a65e5bd0ep+3,  0x1.0dfa4f4feec89p+4},
        {0x1.4a4cab5bdb777p+6, -0x1.1bf0c7950b46ap+3,  0x1.9433415e2e559p+3},
        {0x1.4c9756fc325a6p+6, -0x1.d6e499fe26166p+2,  0x1.1693b52228314p+3},
        {0x1.4fe3e8502420ap+6, -0x1.6b29463056a7ep+2,  0x1.fcea504279e1bp+1},
        {0x1.543e4f84c51c9p+6, -0x1.01c201c3fcfbep+2, -0x1.311b349896504p+0},
        {0x1.59a86b698fe17p+6, -0x1.4697ac30fd276p+1, -0x1.a218614573a54p+2},
        {0x1.6d3f94c53849cp+6, -0x1.e750c3b6abf94p+3,  0x1.2cfd8e3a3298cp+4},
        {0x1.6ee9ffc7c561bp+6, -0x1.b847cb4caca9ap+3,  0x1.ea32a397a5ea3p+3},
        {0x1.70c3ef56a2364p+6, -0x1.8cc812a446223p+3,  0x1.7e0dc7e1aa4eep+3},
        {0x1.737125604c02dp+6, -0x1.5959ff3792892p+3,  0x1.ed9e6884d6db8p+2},
        {0x1.76fef3ccb11e4p+6, -0x1.2379a952d29eap+3,  0x1.77d044e094287p+1},
        {0x1.7b739668b58adp+6, -0x1.def62d62ac5d3p+2, -0x1.1242c0559f9b9p+1},
        {0x1.b9af6705b3e1ap+5,  0x1.3c46b4e4aa724p+4,  0x1.61ea416f62116p+3},
        {0x1.c34652a386648p+5,  0x1.673d5400dac6ap+4,  0x1.575529f864e4ap+1},
        {0x1.cd90a5a7155efp+5,  0x1.7aac661c86347p+4, -0x1.97f5d2781d1d2p+1},
        {0x1.dbc2183aadc6dp+5,  0x1.83f34257f81ecp+4, -0x1.24c7f27451716p+3},
        {0x1.ed84b050fd5fep+5,  0x1.823e0b9a9286fp+4, -0x1.dc10370a896a1p+3},
        {0x1.012d3f7585d8bp+6,  0x1.772916c92c8cp+4,  -0x1.444d46ad4a3f5p+4},
        {0x1.f498a57d50c8dp+5,  0x1.72c50647a1b01p+3,  0x1.9501b7bc92a26p+3},
        {0x1.fbec5add8799fp+5,  0x1.c10937ac1ff0cp+3,  0x1.71506b03a0cfcp+2},
        {0x1.01f4d75996fc6p+6,  0x1.f168c838b27a5p+3,  0x1.a2871b4170f08p-2},
        {0x1.07934868d42f1p+6,  0x1.0ba82dcee8d5fp+4, -0x1.55f50c12fdb69p+2},
        {0x1.0ec4c024626fep+6,  0x1.16b4701bb2cep+4,  -0x1.627c9a1b01177p+3},
        {0x1.1768d8b0aa031p+6,  0x1.199f4e2e9e65fp+4, -0x1.09c1c4e924702p+4},
        {0x1.14522a4a9a7e2p+6,  0x1.6020b0a7ecac7p+2,  0x1.c1a8f99df62d9p+3},
        {0x1.174b930ef061ap+6,  0x1.ec83068a9839fp+2,  0x1.06f134f7cc07ep+3},
        {0x1.1a90921a38fe7p+6,  0x1.28d89eb448a2cp+3,  0x1.ab5fe99a94265p+1},
        {0x1.1f36b9520ad75p+6,  0x1.57ace8397fb96p+3, -0x1.0feb2da7758d4p+1},
        {0x1.253f2b741784fp+6,  0x1.7c087d08384b1p+3, -0x1.efdb49212f992p+2},
        {0x1.2c9a998952cf1p+6,  0x1.9366b5a4edd8ap+3, -0x1.a9848b6099745p+3},
        {0x1.330541ae3e385p+6, -0x1.39e238ea7801p-1,   0x1.f5ec9f058d778p+3},
        {0x1.3568ff8940412p+6,  0x1.4b2aea6a2b17bp+0,  0x1.5aadb9534e15p+3},
        {0x1.380d495b09ffp+6,   0x1.6b2260c162acep+1,  0x1.a0b3e40fdedfp+2},
        {0x1.3bd6db55a86abp+6,  0x1.1c0894632c07ep+2,  0x1.73acfbc73a52cp+0},
        {0x1.40ced5740b6ecp+6,  0x1.784d8d5bd59bap+2, -0x1.f87f29ae0432ap+1},
        {0x1.46f1d56d6a627p+6,  0x1.c2acec311d85p+2,  -0x1.2d078d1d0ec93p+3},
        {0x1.54692a261ff77p+6, -0x1.91aafde5841e2p+2,  0x1.1733b7c5acf09p+4},
        {0x1.5657fb0185f53p+6, -0x1.292fef1c6094ap+2,  0x1.adacb9827ba1ep+3},
        {0x1.587cf8c168362p+6, -0x1.9aa045411de21p+1,  0x1.34f47fa15abacp+3},
        {0x1.5b946ca219efdp+6, -0x1.98465c0dc7b2ap+0,  0x1.43ded9d06d2f2p+2},
        {0x1.5faadb5249c01p+6, -0x1.d35a0bdd6ap-11,   -0x1.f008c176226p-8},
        {0x1.64c3b31613c93p+6,  0x1.6ffd305ac700dp+0, -0x1.52580b551209ep+2},
        {0x1.773c645e32d2dp+6, -0x1.6c001f7fba478p+3,  0x1.3482f283026fp+4},
        {0x1.78d215b19332bp+6, -0x1.3f6c1cce8d703p+3,  0x1.fda06ae8dbf53p+3},
        {0x1.7a9531dc8addap+6, -0x1.15dd42d16c7e4p+3,  0x1.94e8098e96fccp+3},
        {0x1.7d21e2193d5bcp+6, -0x1.c8c718caa9f8ep+2,  0x1.1105d830d79dp+3},
        {0x1.808572bc9578ap+6, -0x1.607bf910c57eep+2,  0x1.ebe2b93545493p+1},
        {0x1.84c6a7914070fp+6, -0x1.f6c2ba3dfc25ap+1, -0x1.2beab2de5e36ep+0},
        {0x1.f663baac570efp+5,  0x1.67be9f690c994p+4,  0x1.928e76c3750aep+3},
        {0x1.fdd78bfa0c16bp+5,  0x1.8da11d29326eap+4,  0x1.63128e5e5d16fp+2},
        {0x1.02fa8211ca545p+6,  0x1.a3a4c44447321p+4,  0x1.04d0a318f0ebp-3},
        {0x1.08ace6a6b1bbcp+6,  0x1.b32f97fdc6c72p+4, -0x1.69ea7f28a916ap+2},
        {0x1.0ff3cb8943e81p+6,  0x1.b9bf03db71bbbp+4, -0x1.6cb54bc7e9c6p+3},
        {0x1.18ac5c68852cdp+6,  0x1.b753a9bd5dbeep+4, -0x1.0ebf0dff35bfbp+4},
        {0x1.12e57190e4906p+6,  0x1.f7243f8456bc9p+3,  0x1.bbbf019d6f15fp+3},
        {0x1.15f567ae01685p+6,  0x1.1e238872ebfe1p+4,  0x1.f5db81101607bp+2},
        {0x1.1952176f81196p+6,  0x1.35eb43e1b3454p+4,  0x1.6f044118da651p+1},
        {0x1.1e17b3f5be1e9p+6,  0x1.4aa233635e996p+4, -0x1.54536023af51fp+1},
        {0x1.244540fde90c8p+6,  0x1.590428a4cd957p+4, -0x1.09fd595c81312p+3},
        {0x1.2bc8d270727cdp+6,  0x1.6002d76c0fae1p+4, -0x1.bbbb8dd0769e1p+3},
        {0x1.2910051d7f4b8p+6,  0x1.4505f3d8c93bep+3,  0x1.e1f0441f64aep+3},
        {0x1.2ba848891bba1p+6,  0x1.83d80a13be75dp+3,  0x1.39f759ee811ebp+3},
        {0x1.2e852380cafc2p+6,  0x1.b3d439de20201p+3,  0x1.5018eb1694a48p+2},
        {0x1.329c951f395aap+6,  0x1.e28db45c54996p+3,  0x1.689e197414bp-7},
        {0x1.37f5013da44b7p+6,  0x1.04846845ce376p+4, -0x1.5f9623aa14c25p+2},
        {0x1.3e86c9cea44c3p+6,  0x1.11f59319394fdp+4, -0x1.6049926fe33dap+3},
        {0x1.443efb939f1d1p+6,  0x1.0a131d35fab88p+2,  0x1.0838884c6c35ap+4},
        {0x1.4667729b66dbap+6,  0x1.793a281884d1p+2,   0x1.8155ad5254ba1p+3},
        {0x1.48cb51f255527p+6,  0x1.d609d14c020d1p+2,  0x1.fd82e0735fdcep+2},
        {0x1.4c3af36c4bb13p+6,  0x1.1c709781a9fb2p+3,  0x1.8ca66dc4e33fdp+1},
        {0x1.50c14ed0ba88bp+6,  0x1.4a1d653379591p+3, -0x1.14ee52d3fd0e1p+1},
        {0x1.565e87fd89a8p+6,   0x1.6fe5c4c37980fp+3, -0x1.e3e3e3d81f648p+2},
        {0x1.62b687ada6a2dp+6, -0x1.b1a7c98c53b9bp+0,  0x1.221d2fc2be6cp+4},
        {0x1.6480f342fedfap+6, -0x1.728e49c5a9ba8p-3,  0x1.cb44060a17de8p+3},
        {0x1.667e081a8e931p+6,  0x1.2c204353d8acp+0,   0x1.582d07a1ac706p+3},
        {0x1.695d03a4b4ea6p+6,  0x1.5b9dd1e5192c9p+1,  0x1.949cc8da7db3dp+2},
        {0x1.6d2ad25b49139p+6,  0x1.10c661536b1e3p+2,  0x1.60b9aee667ea2p+0},
        {0x1.71ec510363cccp+6,  0x1.6b2af260ca3f1p+2, -0x1.e8e3d6769ed61p+1},
        {0x1.83323c984ee7ap+6, -0x1.c8df58716d4cbp+2,  0x1.3d9335b5d20f9p+4},
        {0x1.84b109344c92cp+6, -0x1.750134f94569cp+2,  0x1.0a63166d3ca32p+4},
        {0x1.865ae5967696dp+6, -0x1.260f129da839ep+2,  0x1.b00f6d0ac1f93p+3},
        {0x1.88c3891dbffdp+6,  -0x1.8e7bf5d542ea4p+1,  0x1.30323669c3d67p+3},
        {0x1.8bf825267af8ep+6, -0x1.89d8c0f2ba1ffp+0,  0x1.3b2af0c9eed76p+2},
        {0x1.8ffffbb77c76ap+6, -0x1.09cab717214p-10,  -0x1.1a1aa7765a7p-7},
        {0x1.ae1c063cf8075p+3, -0x1.1dcc8d6b21p-13,   -0x1.2f56d49352fp-10},
        {0x1.23869fde6955fp+4, -0x1.836d13c82dp-13,   -0x1.9b340cb2926p-10},
        {0x1.6a51d9755cb1ep+4, -0x1.e1821bf6d08p-13,  -0x1.ff0f3c5806ap-10},
        {0x1.adca073d0c9a1p+4, -0x1.1d961152df8p-12,  -0x1.2f1d00745cap-9},
        {0x1.eeb26a3638306p+4, -0x1.48b751f0a58p-12,  -0x1.5ce3e1a3db8p-9},
        {0x1.16c4868a9dbc4p+5, -0x1.7278906708p-12,   -0x1.89352628678p-9},
        {0x1.3552cb4726ed2p+5, -0x1.9b140ac6fbp-12,   -0x1.b44e9f2325p-9},
        {0x1.53242132979b2p+5, -0x1.c2b46f9c328p-12,  -0x1.de5d99b6f9p-9},
        {0x1.7051013cf5a69p+5, -0x1.e97a44c10a8p-12,  -0x1.03c24d5b897p-8},
        {0x1.8ceca3a0569fdp+5, -0x1.07bf8a7e0fcp-11,  -0x1.17ef5f1c441p-8},
        {0x1.a9067ed63306p+5,  -0x1.1a6bb67a6bp-11,   -0x1.2bc0e9df0a8p-8},
        {0x1.c4ab42f91535bp+5, -0x1.2cca14a233p-11,   -0x1.3f3fe0662d2p-8},
        {0x1.dfe5855ae1528p+5, -0x1.3ee1ad40618p-11,  -0x1.5273b9784a3p-8},
        {0x1.fabe397d15c9dp+5, -0x1.50b872fc158p-11,  -0x1.6562c531248p-8},
        {0x1.0a9e8459f8d9ap+6, -0x1.62537d1958p-11,   -0x1.78126ad030ep-8},
        {0x1.17b44990f13f5p+6, -0x1.73b732a42fp-11,   -0x1.8a87568ccdp-8},
        {0x1.24a350705ddf4p+6, -0x1.84e76b1c308p-11,  -0x1.9cc59c424aap-8},
        {0x1.316e23ed9d96ap+6, -0x1.95e7879947p-11,   -0x1.aed0d2216dp-8},
        {0x1.3e1704b29c069p+6, -0x1.a6ba8679e08p-11,  -0x1.c0ac258f992p-8},
        {0x1.4a9ff4d8984e6p+6, -0x1.b76312f3808p-11,  -0x1.d25a6bb27acp-8},
        {0x1.570ac151c1615p+6, -0x1.c7e3919c138p-11,  -0x1.e3de2eb684p-8},
        {0x1.6359098c40c61p+6, -0x1.d83e2a89bbp-11,   -0x1.f539b894d66p-8},
        {0x1.6f8c45b456692p+6, -0x1.e874d1a7f5p-11,   -0x1.03378df3804p-7},
        {0x1.7ba5cbe12c3fcp+6, -0x1.f8894d9602p-11,   -0x1.0bc01d99c93p-7},
    };

    // Perform the inverse gamma companding for a sRGB color
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_to_XYZ.html
    // double inverseGammaCompanding(int c) {
    //     // [0, 255] to [0, 1]
    //     double v = c / 255.0;
    //     if (v <= 0.04045) {
    //         return v / 12.92;
    //     }
    //     return std::pow((v + 0.055) / 1.055, 2.4);
    // };
    constexpr double RGB888_to_sRGB_table[] {
        0x0p+0,                0x1.3e45677c176f7p-12, 0x1.3e45677c176f7p-11, 0x1.dd681b3a23272p-11,
        0x1.3e45677c176f7p-10, 0x1.8dd6c15b1d4b4p-10, 0x1.dd681b3a23272p-10, 0x1.167cba8c94818p-9,
        0x1.3e45677c176f7p-9,  0x1.660e146b9a5d6p-9,  0x1.8dd6c15b1d4b4p-9,  0x1.b6a31b5259c99p-9,
        0x1.e1e31d70c99ddp-9,  0x1.07c38bf8583a9p-8,  0x1.1fcc2beed6421p-8,  0x1.390ffaf95e279p-8,
        0x1.53936cc7bc928p-8,  0x1.6f5addb50c915p-8,  0x1.8c6a94031b561p-8,  0x1.aac6c0fb97351p-8,
        0x1.ca7381f9f602bp-8,  0x1.eb74e160978dp-8,   0x1.06e76bbda92b8p-7,  0x1.18c2a5a8a8044p-7,
        0x1.2b4e09b3f0ae3p-7,  0x1.3e8b7b3bde965p-7,  0x1.527cd60af8b85p-7,  0x1.6723eea8d3709p-7,
        0x1.7c8292a3db6b3p-7,  0x1.929a88d67b521p-7,  0x1.a96d91a8016bdp-7,  0x1.c0fd67499fab6p-7,
        0x1.d94bbdefd740ep-7,  0x1.f25a44089883fp-7,  0x1.061551372c694p-6,  0x1.135f3e4c2cce2p-6,
        0x1.210bb8642b172p-6,  0x1.2f1b8c1ae46bdp-6,  0x1.3d8f839b79c0bp-6,  0x1.4c6866b3e9fa4p-6,
        0x1.5ba6fae794313p-6,  0x1.6b4c0380d2deep-6,  0x1.7b5841a1bf3acp-6,  0x1.8bcc74542addbp-6,
        0x1.9ca95898dc8b5p-6,  0x1.adefa9761c02p-6,   0x1.bfa0200597bd9p-6,  0x1.d1bb7381aec1fp-6,
        0x1.e442595227bcap-6,  0x1.f73585185e1b5p-6,  0x1.054ad45d76878p-5,  0x1.0f31ba386ff26p-5,
        0x1.194fcb663747bp-5,  0x1.23a55e62a662ap-5,  0x1.2e32c8e148d11p-5,  0x1.38f85fd21eacfp-5,
        0x1.43f67766310ffp-5,  0x1.4f2d6313fa8dp-5,   0x1.5a9d759ba5edp-5,   0x1.6647010b254eep-5,
        0x1.722a56c2239eep-5,  0x1.7e47c775d2427p-5,  0x1.8a9fa33494b07p-5,  0x1.973239698b9ccp-5,
        0x1.a3ffd8e001389p-5,  0x1.b108cfc6b7fbcp-5,  0x1.be4d6bb31d522p-5,  0x1.cbcdf9a4616f2p-5,
        0x1.d98ac60675833p-5,  0x1.e7841cb4f16dfp-5,  0x1.f5ba48fde2048p-5,  0x1.0216cad240765p-4,
        0x1.096f2671eb815p-4,  0x1.10e65c38a5192p-4,  0x1.187c90bf8bce2p-4,  0x1.2031e85f5d6dap-4,
        0x1.28068731a1952p-4,  0x1.2ffa9111cb94bp-4,  0x1.380e299e53f92p-4,  0x1.40417439ca10fp-4,
        0x1.4894940bddbfbp-4,  0x1.5107ac0261e59p-4,  0x1.599aded247aacp-4,  0x1.624e4ef892ed4p-4,
        0x1.6b221ebb4817ep-4,  0x1.7416702a539d1p-4,  0x1.7d2b65206b527p-4,  0x1.86611f43e9e6ap-4,
        0x1.8fb7c007a4a7p-4,   0x1.992f68abbbc89p-4,  0x1.a2c83a3e6566dp-4,  0x1.ac82559cb3644p-4,
        0x1.b65ddb7354604p-4,  0x1.c05aec3f4fe5ep-4,  0x1.ca79a84ebe03p-4,   0x1.d4ba2fc17a6a5p-4,
        0x1.df1ca289d34b8p-4,  0x1.e9a1206d34003p-4,  0x1.f447c904cbb4ep-4,  0x1.ff10bbbe302c2p-4,
        0x1.04fe0bedfe5f1p-3,  0x1.0a84fe3b36d8fp-3,  0x1.101d443dfc06fp-3,  0x1.15c6ed58eefdfp-3,
        0x1.1b8208da5fefp-3,   0x1.214ea5fc9514ap-3,  0x1.272cd3e610123p-3,  0x1.2d1ca1a9d1cfbp-3,
        0x1.331e1e479cdf5p-3,  0x1.393158ac3674ep-3,  0x1.3f565fb1a5fd5p-3,  0x1.458d421f735dfp-3,
        0x1.4bd60eaae3e73p-3,  0x1.5230d3f736034p-3,  0x1.589da095dbaa1p-3,  0x1.5f1c8306b3a3cp-3,
        0x1.65ad89b841a2bp-3,  0x1.6c50c307e53bfp-3,  0x1.73063d420fc8p-3,   0x1.79ce06a279303p-3,
        0x1.80a82d5453b5dp-3,  0x1.8794bf727eb3fp-3,  0x1.8e93cb07b8679p-3,  0x1.95a55e0ecec0bp-3,
        0x1.9cc98672cf47ep-3,  0x1.a400520f3619cp-3,  0x1.ab49ceb01c003p-3,  0x1.b2a60a1263b0ap-3,
        0x1.ba1511e3e632dp-3,  0x1.c196f3c39e76fp-3,  0x1.c92bbd41d41fep-3,  0x1.d0d37be045851p-3,
        0x1.d88e3d1250f68p-3,  0x1.e05c0e3d1d3ep-3,   0x1.e83cfcb7c16fp-3,   0x1.f03115cb6bfd3p-3,
        0x1.f83866b38924dp-3,  0x1.00297e4ef4553p-2,  0x1.044072557177ap-2,  0x1.086115f6beb3ap-2,
        0x1.0c8b6fb5c735ep-2,  0x1.10bf860ef039ap-2,  0x1.14fd5f782a5a6p-2,  0x1.1945026102997p-2,
        0x1.1d967532b31b1p-2,  0x1.21f1be50339e7p-2,  0x1.2656e41649ae3p-2,  0x1.2ac5ecdb988f8p-2,
        0x1.2f3edef0b0ed8p-2,  0x1.33c1c0a020438p-2,  0x1.384e982e800b1p-2,  0x1.3ce56bda84a81p-2,
        0x1.418641dd0c1bcp-2,  0x1.463120692c7afp-2,  0x1.4ae60dac4229dp-2,  0x1.4fa50fcdfde15p-2,
        0x1.546e2cf0727a9p-2,  0x1.59416b3022858p-2,  0x1.5e1ed0a40daabp-2,  0x1.6306635dbdd7bp-2,
        0x1.67f82969543a2p-2,  0x1.6cf428cd96079p-2,  0x1.71fa678bf915dp-2,  0x1.770aeba0b042ap-2,
        0x1.7c25bb02b7ac5p-2,  0x1.814adba3e0bd9p-2,  0x1.867a5370de0b1p-2,  0x1.8bb428514f067p-2,
        0x1.90f86027cb84ep-2,  0x1.964700d1ef1b1p-2,  0x1.9ba0102864521p-2,  0x1.a10393feefafdp-2,
        0x1.a67192247a9bep-2,  0x1.abea10631e195p-2,  0x1.b16d14802d5cap-2,  0x1.b6faa43c403bbp-2,
        0x1.bc92c5533d785p-2,  0x1.c2357d7c64e5dp-2,  0x1.c7e2d26a596dep-2,  0x1.cd9ac9cb2aef2p-2,
        0x1.d35d69485ffc5p-2,  0x1.d92ab686ff782p-2,  0x1.df02b7279a10dp-2,  0x1.e4e570c6539c5p-2,
        0x1.ead2e8faec526p-2,  0x1.f0cb2558c9ea4p-2,  0x1.f6ce2b6f00983p-2,  0x1.fcdc00c85bec2p-2,
        0x1.017a5575b3cb2p-1,  0x1.048c17ad3c04bp-1,  0x1.07a349c9d9837p-1,  0x1.0abfee888c05p-1,
        0x1.0de208a4444c8p-1,  0x1.11099ad5e83ebp-1,  0x1.1436a7d456eefp-1,  0x1.176932546ca12p-1,
        0x1.1aa13d0906bdap-1,  0x1.1ddecaa307b85p-1,  0x1.2121ddd15aecep-1,  0x1.246a7940f86d1p-1,
        0x1.27b89f9ce8c4bp-1,  0x1.2b0c538e48b07p-1,  0x1.2e6597bc4ccap-1,   0x1.31c46ecc4528dp-1,
        0x1.3528db61a0f73p-1,  0x1.3892e01df1fccp-1,  0x1.3c027fa0f01ebp-1,  0x1.3f77bc887cd3bp-1,
        0x1.42f29970a68f8p-1,  0x1.467318f3ac22dp-1,  0x1.49f93daa00113p-1,  0x1.4d850a2a4bde1p-1,
        0x1.51168109734e5p-1,  0x1.54ada4da97a1bp-1,  0x1.584a782f1ac23p-1,  0x1.5becfd96a2698p-1,
        0x1.5f95379f1b3edp-1,  0x1.634328d4bbe97p-1,  0x1.66f6d3c2081cfp-1,  0x1.6ab03aefd39aap-1,
        0x1.6e6f60e5452b1p-1,  0x1.72344827d98f6p-1,  0x1.75fef33b6669bp-1,  0x1.79cf64a21d1e2p-1,
        0x1.7da59edc8dabp-1,   0x1.8181a469a9787p-1,  0x1.856377c6c6224p-1,  0x1.894b1b6fa0377p-1,
        0x1.8d3891de5df49p-1,  0x1.912bdd8b91f45p-1,  0x1.952500ee3dda5p-1,  0x1.9923fe7bd4f67p-1,
        0x1.9d28d8a83edfcp-1,  0x1.a13391e5da09fp-1,  0x1.a5442ca57e52ep-1,  0x1.a95aab567f88fp-1,
        0x1.ad771066afec2p-1,  0x1.b1995e4262a69p-1,  0x1.b5c197546e3f8p-1,  0x1.b9efbe062f086p-1,
        0x1.be23d4bf8981bp-1,  0x1.c25ddde6ecbbbp-1,  0x1.c69ddbe154af1p-1,  0x1.cae3d1124c90bp-1,
        0x1.cf2fbfdbf11f1p-1,  0x1.d381aa9ef2e82p-1,  0x1.d7d993ba988d4p-1,  0x1.dc377d8cc0fd5p-1,
        0x1.e09b6a71e5aa6p-1,  0x1.e5055cc51cbb4p-1,  0x1.e97556e01b351p-1,  0x1.edeb5b1b37216p-1,
        0x1.f2676bcd69adep-1,  0x1.f6e98b4c51466p-1,  0x1.fb71bbec33ab2p-1,  0x1p+0,
    };
// clang-format on

// convert a RGB color to Oklab (https://bottosson.github.io/posts/oklab/)
Lab rgbToOklab(QRgb rgb)
{
    const double r = RGB888_to_sRGB_table[qRed(rgb)];
    const double g = RGB888_to_sRGB_table[qGreen(rgb)];
    const double b = RGB888_to_sRGB_table[qBlue(rgb)];

    const double l = std::cbrt(0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b);
    const double m = std::cbrt(0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b);
    const double s = std::cbrt(0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b);

    // M2 * 100 * {l', m', s'}
    return Lab{
        (021.04542553 * l + 079.36177850 * m - 000.40720468 * s),
        (197.79984951 * l - 242.85922050 * m + 045.05937099 * s),
        (002.59040371 * l + 078.27717662 * m - 080.86757660 * s),
    };
}

constexpr double epsilon = 1e-15;

inline double sinDegree(double x)
{
    return std::sin(x * M_PI / 180.0);
}

inline double cosDegree(double x)
{
    return std::cos(x * M_PI / 180.0);
}

inline double pow2(double x)
{
    return x * x;
}

inline double computeHPrime(double a_prime, double b)
{
    if (std::abs(a_prime) < epsilon && std::abs(b) < epsilon) {
        return 0.0;
    }

    const double value = std::atan2(b, a_prime) * 180.0 / M_PI;
    return (value < 0.0) ? value + 360.0 : value;
}

inline double computeDeltaHPrime(double C1_prime, double C2_prime, double h1_prime, double h2_prime)
{
    if (C1_prime * C2_prime < epsilon) {
        return 0.0;
    }

    const double diff = h2_prime - h1_prime;

    if (std::abs(diff) <= 180.0) {
        return diff;
    } else if (diff > 180.0) {
        return diff - 360.0;
    } else {
        return diff + 360.0;
    }
}

inline double computeHPrimeBar(double C1_prime, double C2_prime, double h1_prime, double h2_prime)
{
    const double sum = h1_prime + h2_prime;

    if (C1_prime * C2_prime < epsilon) {
        return sum;
    }

    const double dist = std::abs(h1_prime - h2_prime);

    if (dist <= 180.0) {
        return 0.5 * sum;
    } else if (sum < 360.0) {
        return 0.5 * (sum + 360.0);
    } else {
        return 0.5 * (sum - 360.0);
    }
}

/// Calculate the perceptual color difference based on CIEDE2000.
/// https://en.wikipedia.org/wiki/Color_difference#CIEDE2000
/// \return The color difference of the two colors.
double calculate_CIEDE2000(const Lab &color1, const Lab &color2)
{
    const double L1 = color1.L;
    const double a1 = color1.a;
    const double b1 = color1.b;
    const double L2 = color2.L;
    const double a2 = color2.a;
    const double b2 = color2.b;

    const double _25_pow_7 = /*std::pow(25.0, 7.0) = */ 6103515625.0;

    const double C1_ab = std::sqrt(a1 * a1 + b1 * b1);
    const double C2_ab = std::sqrt(a2 * a2 + b2 * b2);
    const double C_ab_bar = 0.5 * (C1_ab + C2_ab);
    const double c_ab_bar_pow_7 = std::pow(C_ab_bar, 7.0);
    const double G = 0.5 * (1.0 - std::sqrt(c_ab_bar_pow_7 / (c_ab_bar_pow_7 + _25_pow_7)));
    const double a1_prime = (1.0 + G) * a1;
    const double a2_prime = (1.0 + G) * a2;
    const double C1_prime = std::sqrt(a1_prime * a1_prime + b1 * b1);
    const double C2_prime = std::sqrt(a2_prime * a2_prime + b2 * b2);
    const double h1_prime = computeHPrime(a1_prime, b1);
    const double h2_prime = computeHPrime(a2_prime, b2);

    const double deltaL_prime = L2 - L1;
    const double deltaC_prime = C2_prime - C1_prime;
    const double deltah_prime = computeDeltaHPrime(C1_prime, C2_prime, h1_prime, h2_prime);
    const double deltaH_prime = 2.0 * std::sqrt(C1_prime * C2_prime) * sinDegree(0.5 * deltah_prime);

    const double L_primeBar = 0.5 * (L1 + L2);
    const double C_primeBar = 0.5 * (C1_prime + C2_prime);
    const double h_primeBar = computeHPrimeBar(C1_prime, C2_prime, h1_prime, h2_prime);

    const double T = 1.0 - 0.17 * cosDegree(h_primeBar - 30.0) + 0.24 * cosDegree(2.0 * h_primeBar) + 0.32 * cosDegree(3.0 * h_primeBar + 6.0)
        - 0.20 * cosDegree(4.0 * h_primeBar - 63.0);

    const double C_primeBar_pow7 = std::pow(C_primeBar, 7.0);
    const double R_C = 2.0 * std::sqrt(C_primeBar_pow7 / (C_primeBar_pow7 + _25_pow_7));
    const double S_L = 1.0 + (0.015 * pow2(L_primeBar - 50.0)) / std::sqrt(20.0 + pow2(L_primeBar - 50.0));
    const double S_C = 1.0 + 0.045 * C_primeBar;
    const double S_H = 1.0 + 0.015 * C_primeBar * T;
    const double R_T = -R_C * sinDegree(60.0 * std::exp(-pow2((h_primeBar - 275) / 25.0)));

    constexpr double kL = 1.0;
    constexpr double kC = 1.0;
    constexpr double kH = 1.0;

    const double deltaL = deltaL_prime / (kL * S_L);
    const double deltaC = deltaC_prime / (kC * S_C);
    const double deltaH = deltaH_prime / (kH * S_H);

    return /*std::sqrt*/ (deltaL * deltaL + deltaC * deltaC + deltaH * deltaH + R_T * deltaC * deltaH);
}

struct AnsiBuffer {
    using ColorCache = QHash<QRgb, int>;

    void append(char c)
    {
        Q_ASSERT(remaining() >= 1);
        m_data[m_size] = c;
        ++m_size;
    }

    void append(QLatin1String str)
    {
        Q_ASSERT(remaining() >= str.size());
        memcpy(m_data + m_size, str.data(), str.size());
        m_size += str.size();
    }

    void appendForeground(QRgb rgb, bool is256Colors, ColorCache &colorCache)
    {
        append(QLatin1String("38;"));
        append(rgb, is256Colors, colorCache);
    }

    void appendBackground(QRgb rgb, bool is256Colors, ColorCache &colorCache)
    {
        append(QLatin1String("48;"));
        append(rgb, is256Colors, colorCache);
    }

    void append(QRgb rgb, bool is256Colors, ColorCache &colorCache)
    {
        auto appendUInt8 = [&](int x) {
            Q_ASSERT(x <= 255 && x >= 0);
            if (x > 99) {
                if (x >= 200) {
                    append('2');
                    x -= 200;
                } else {
                    append('1');
                    x -= 100;
                }
            } else if (x < 10) {
                append(char('0' + x));
                return;
            }

            // clang-format off
            constexpr char const* tb2digits =
                "00" "01" "02" "03" "04" "05" "06" "07" "08" "09"
                "10" "11" "12" "13" "14" "15" "16" "17" "18" "19"
                "20" "21" "22" "23" "24" "25" "26" "27" "28" "29"
                "30" "31" "32" "33" "34" "35" "36" "37" "38" "39"
                "40" "41" "42" "43" "44" "45" "46" "47" "48" "49"
                "50" "51" "52" "53" "54" "55" "56" "57" "58" "59"
                "60" "61" "62" "63" "64" "65" "66" "67" "68" "69"
                "70" "71" "72" "73" "74" "75" "76" "77" "78" "79"
                "80" "81" "82" "83" "84" "85" "86" "87" "88" "89"
                "90" "91" "92" "93" "94" "95" "96" "97" "98" "99";
            // clang-format on

            auto *p = tb2digits + x * 2;
            append(p[0]);
            append(p[1]);
        };

        if (is256Colors) {
            double dist = 1e24;
            int idx = 0;
            auto it = colorCache.find(rgb);
            if (it == colorCache.end()) {
                const auto lab = rgbToOklab(rgb);
                // find the nearest xterm color
                for (Lab const &xtermLab : xterm240_Oklabs) {
                    auto dist2 = calculate_CIEDE2000(lab, xtermLab);
                    if (dist2 < dist) {
                        dist = dist2;
                        idx = &xtermLab - xterm240_Oklabs;
                    }
                }
                // add 16 to convert 240 colors mode to 256 colors mode
                idx += 16;
                colorCache.insert(rgb, idx);
            } else {
                idx = it.value();
            }

            append('5');
            append(';');
            appendUInt8(idx);
        } else {
            append('2');
            append(';');
            appendUInt8(qRed(rgb));
            append(';');
            appendUInt8(qGreen(rgb));
            append(';');
            appendUInt8(qBlue(rgb));
        }
        append(';');
    }

    // Replace last character with 'm'. Last character must be ';'
    void setFinalStyle()
    {
        Q_ASSERT(m_data[m_size - 1] == ';');
        m_data[m_size - 1] = 'm';
    }

    void clear()
    {
        m_size = 0;
    }

    QLatin1String latin1() const
    {
        return QLatin1String(m_data, m_size);
    }

private:
    char m_data[128];
    int m_size = 0;

    int remaining() const noexcept
    {
        return 128 - m_size;
    }
};

void fillString(QString &s, int n, QStringView fill)
{
    if (n > 0) {
        for (; n > fill.size(); n -= fill.size()) {
            s += fill;
        }
        s += fill.left(n);
    }
}

struct GraphLine {
    QString graphLine;
    QString labelLine;
    int graphLineLength = 0;
    int labelLineLength = 0;
    int nextLabelOffset = 0;

    template<class String>
    void pushLabel(int offset, String const &s, int numberDisplayableChar)
    {
        Q_ASSERT(offset >= labelLineLength);
        const int n = offset - labelLineLength;
        labelLineLength += numberDisplayableChar + n;
        fillLine(labelLine, n);
        labelLine += s;
        nextLabelOffset = labelLineLength;
    }

    template<class String>
    void pushGraph(int offset, String const &s, int numberDisplayableChar)
    {
        Q_ASSERT(offset >= graphLineLength);
        const int n = offset - graphLineLength;
        graphLineLength += numberDisplayableChar + n;
        fillLine(graphLine, n);
        const int ps1 = graphLine.size();
        graphLine += s;
        if (offset >= labelLineLength) {
            const int n2 = offset - labelLineLength;
            labelLineLength += n2 + 1;
            fillLine(labelLine, n2);
            labelLine += QStringView(graphLine).right(graphLine.size() - ps1);
        }
    }

private:
    static void fillLine(QString &s, int n)
    {
        Q_ASSERT(n >= 0);
        fillString(s,
                   n,
                   QStringLiteral("                              "
                                  "                              "
                                  "                              "));
    }
};

/**
 * Returns the first free line at a given position or create a new one
 */
GraphLine &lineAtOffset(std::vector<GraphLine> &graphLines, int offset)
{
    const auto last = graphLines.end();
    auto p = std::find_if(graphLines.begin(), last, [=](GraphLine const &line) {
        return line.nextLabelOffset < offset;
    });
    if (p == last) {
        graphLines.emplace_back();
        return graphLines.back();
    }
    return *p;
}

// disable bold, italic and underline on |
const QLatin1String graphSymbol("\x1b[21;23;24m|");
// reverse video
const QLatin1String nameStyle("\x1b[7m");

/**
 * ANSI Highlighter dedicated to traces
 */
class DebugSyntaxHighlighter : public KSyntaxHighlighting::AbstractHighlighter
{
public:
    using TraceOption = KSyntaxHighlighting::AnsiHighlighter::TraceOption;
    using TraceOptions = KSyntaxHighlighting::AnsiHighlighter::TraceOptions;

    void setDefinition(const KSyntaxHighlighting::Definition &def) override
    {
        AbstractHighlighter::setDefinition(def);
        m_contextCapture.setDefinition(def);

        const auto &definitions = def.includedDefinitions();
        for (const auto &definition : definitions) {
            const auto *defData = DefinitionData::get(definition);
            for (const auto &context : defData->contexts) {
                m_defDataBycontexts.insert(&context, defData);
            }
        }
    }

    void highlightData(QTextStream &in,
                       QTextStream &out,
                       QLatin1String infoStyle,
                       QLatin1String editorBackground,
                       const std::vector<QPair<QString, QString>> &ansiStyles,
                       TraceOptions traceOptions)
    {
        initRegionStyles(ansiStyles);

        m_hasFormatTrace = traceOptions.testFlag(TraceOption::Format);
        m_hasRegionTrace = traceOptions.testFlag(TraceOption::Region);
        m_hasStackSizeTrace = traceOptions.testFlag(TraceOption::StackSize);
        m_hasContextTrace = traceOptions.testFlag(TraceOption::Context);
        const bool hasFormatOrContextTrace = m_hasFormatTrace || m_hasContextTrace || m_hasStackSizeTrace;

        const bool hasSeparator = hasFormatOrContextTrace && m_hasRegionTrace;
        const QString resetBgColor = (editorBackground.isEmpty() ? QStringLiteral("\x1b[0m") : editorBackground);

        bool firstLine = true;
        State state;
        QString currentLine;
        while (in.readLineInto(&currentLine)) {
            auto oldState = state;
            state = highlightLine(currentLine, state);

            if (hasSeparator) {
                if (!firstLine) {
                    out << QStringLiteral("\x1b[0m────────────────────────────────────────────────────\x1b[K\n");
                }
                firstLine = false;
            }

            if (!m_regions.empty()) {
                printRegions(out, infoStyle, currentLine.size());
                out << resetBgColor;
            }

            for (const auto &fragment : m_highlightedFragments) {
                auto const &ansiStyle = ansiStyles[fragment.formatId];
                out << ansiStyle.first << QStringView(currentLine).mid(fragment.offset, fragment.length) << ansiStyle.second;
            }

            out << QStringLiteral("\x1b[K\n");

            if (hasFormatOrContextTrace && !m_highlightedFragments.empty()) {
                if (m_hasContextTrace || m_hasStackSizeTrace) {
                    appendContextNames(oldState, currentLine);
                }

                printFormats(out, infoStyle, ansiStyles);
                out << resetBgColor;
            }

            m_highlightedFragments.clear();
        }
    }

    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override
    {
        m_highlightedFragments.push_back({m_hasFormatTrace ? format.name() : QString(), offset, length, format.id()});
    }

    void applyFolding(int offset, int /*length*/, FoldingRegion region) override
    {
        if (!m_hasRegionTrace) {
            return;
        }

        const auto id = region.id();

        if (region.type() == FoldingRegion::Begin) {
            m_regions.push_back(Region{m_regionDepth, offset, -1, id, Region::State::Open});
            // swap with previous region if this is a closing region with same offset in order to superimpose labels
            if (m_regions.size() >= 2) {
                auto &previousRegion = m_regions[m_regions.size() - 2];
                if (previousRegion.state == Region::State::Close && previousRegion.offset == offset) {
                    std::swap(previousRegion, m_regions.back());
                    if (previousRegion.bindIndex != -1) {
                        m_regions[previousRegion.bindIndex].bindIndex = m_regions.size() - 1;
                    }
                }
            }
            ++m_regionDepth;
        } else {
            // find open region
            auto it = m_regions.rbegin();
            auto eit = m_regions.rend();
            for (int depth = 0; it != eit; ++it) {
                if (it->regionId == id && it->bindIndex < 0) {
                    if (it->state == Region::State::Close) {
                        ++depth;
                    } else if (--depth < 0) {
                        break;
                    }
                }
            }

            if (it != eit) {
                it->bindIndex = int(m_regions.size());
                int bindIndex = int(&*it - m_regions.data());
                m_regions.push_back(Region{it->depth, offset, bindIndex, id, Region::State::Close});
            } else {
                m_regions.push_back(Region{-1, offset, -1, id, Region::State::Close});
            }

            m_regionDepth = std::max(m_regionDepth - 1, 0);
        }
    }

    using KSyntaxHighlighting::AbstractHighlighter::highlightLine;

private:
    /**
     * Initializes with colors of \p ansiStyle without duplicate.
     */
    void initRegionStyles(const std::vector<QPair<QString, QString>> &ansiStyles)
    {
        m_regionStyles.resize(ansiStyles.size());
        for (std::size_t i = 0; i < m_regionStyles.size(); ++i) {
            m_regionStyles[i] = ansiStyles[i].first;
        }

        std::sort(m_regionStyles.begin(), m_regionStyles.end());
        m_regionStyles.erase(std::unique(m_regionStyles.begin(), m_regionStyles.end()), m_regionStyles.end());
    }

    /**
     * Append the context name in front of the format name.
     */
    void appendContextNames(State &state, QStringView currentLine)
    {
        auto newState = state;
        for (auto &fragment : m_highlightedFragments) {
            QString contextName = extractContextName(StateData::get(newState));

            m_contextCapture.offsetNext = 0;
            m_contextCapture.lengthNext = 0;
            // truncate the line to deduce the context from the format
            const auto lineFragment = currentLine.mid(0, fragment.offset + fragment.length + 1);
            newState = m_contextCapture.highlightLine(lineFragment, state);

            // Deduced context does not start at the position of the format.
            // This can happen because of lookAhead/fallthrought attribute,
            // assertion in regex, etc.
            if (m_contextCapture.offset != fragment.offset && m_contextCapture.length != fragment.length) {
                contextName.insert(0, QLatin1Char('~'));
            }
            fragment.name.insert(0, contextName);
        }
    }

    /**
     * \return Current context name with definition name if different
     * from the current definition name
     */
    QString extractContextName(StateData *stateData) const
    {
        QString label;

        if (m_hasStackSizeTrace) {
            label += QLatin1Char('(') % QString::number(stateData->size()) % QLatin1Char(')');
        }

        if (m_hasContextTrace) {
            // first state is empty
            if (stateData->isEmpty()) {
                return label + QStringLiteral("[???]");
            }

            const auto context = stateData->topContext();
            const auto defDataIt = m_defDataBycontexts.find(context);
            const auto contextName = (defDataIt != m_defDataBycontexts.end()) ? QString(QLatin1Char('<') % (*defDataIt)->name % QLatin1Char('>')) : QString();
            return QString(label % contextName % QLatin1Char('[') % context->name() % QLatin1Char(']'));
        }

        return label;
    }

    void printFormats(QTextStream &out, QLatin1String regionStyle, const std::vector<QPair<QString, QString>> &ansiStyles)
    {
        // init graph
        m_formatGraph.clear();
        for (auto const &fragment : m_highlightedFragments) {
            GraphLine &line = lineAtOffset(m_formatGraph, fragment.offset);
            auto const &style = ansiStyles[fragment.formatId].first;
            line.pushLabel(fragment.offset, style % nameStyle % fragment.name % regionStyle, fragment.name.size());

            for (GraphLine *pline = m_formatGraph.data(); pline <= &line; ++pline) {
                pline->pushGraph(fragment.offset, style % graphSymbol % regionStyle, 1);
            }
        }

        // display graph
        out << regionStyle;
        auto first = m_formatGraph.begin();
        auto last = m_formatGraph.end();
        --last;
        for (; first != last; ++first) {
            out << first->graphLine << "\x1b[K\n" << first->labelLine << "\x1b[K\n";
        }
        out << first->graphLine << "\x1b[K\n" << first->labelLine << "\x1b[K\x1b[0m\n";
    }

    void printRegions(QTextStream &out, QLatin1String regionStyle, int lineLength)
    {
        const QString continuationLine = QStringLiteral(
            "------------------------------"
            "------------------------------"
            "------------------------------");

        bool hasContinuation = false;

        m_regionGraph.clear();
        QString label;
        QString numStr;

        // init graph
        for (Region &region : m_regions) {
            if (region.state == Region::State::Continuation) {
                hasContinuation = true;
                continue;
            }

            auto pushGraphs = [&](int offset, const GraphLine *endline, QStringView style) {
                for (GraphLine *pline = m_regionGraph.data(); pline <= endline; ++pline) {
                    // a label can hide a graph
                    if (pline->graphLineLength <= offset) {
                        pline->pushGraph(offset, style % graphSymbol % regionStyle, 1);
                    }
                }
            };

            QChar openChar;
            QChar closeChar;
            int lpad = 0;
            int rpad = 0;

            int offsetLabel = region.offset;

            numStr.setNum(region.regionId);

            if (region.state == Region::State::Open) {
                openChar = QLatin1Char('(');
                if (region.bindIndex == -1) {
                    rpad = lineLength - region.offset - numStr.size();
                } else {
                    rpad = m_regions[region.bindIndex].offset - region.offset - 2;
                    closeChar = QLatin1Char(')');
                }
                // close without open
            } else if (region.bindIndex == -1) {
                closeChar = QLatin1Char('>');
                // label already present, we only display the graph
            } else if (m_regions[region.bindIndex].state == Region::State::Open) {
                const auto &openRegion = m_regions[region.bindIndex];
                // here offset is a graph index
                const GraphLine &line = m_regionGraph[openRegion.offset];
                const auto &style = m_regionStyles[openRegion.depth % m_regionStyles.size()];
                pushGraphs(region.offset, &line, style);
                continue;
            } else {
                closeChar = QLatin1Char(')');
                lpad = region.offset - numStr.size();
                offsetLabel = 0;
            }

            const QStringView openS(&openChar, openChar.unicode() ? 1 : 0);
            const QStringView closeS(&closeChar, closeChar.unicode() ? 1 : 0);

            label.clear();
            fillString(label, lpad, continuationLine);
            label += numStr;
            fillString(label, rpad, continuationLine);

            GraphLine &line = lineAtOffset(m_regionGraph, offsetLabel);
            const auto &style = m_regionStyles[region.depth % m_regionStyles.size()];
            line.pushLabel(offsetLabel, style % nameStyle % openS % label % closeS % regionStyle, label.size() + openS.size() + closeS.size());
            pushGraphs(region.offset, &line, style);

            // transforms offset into graph index when region is on 1 line
            if (region.state == Region::State::Open && region.bindIndex != -1) {
                region.offset = &line - m_regionGraph.data();
            }
        }

        out << regionStyle;

        // display regions which are neither closed nor open
        if (hasContinuation) {
            label.clear();
            fillString(label, lineLength ? lineLength : 5, continuationLine);
            for (const auto &region : m_regions) {
                if (region.state == Region::State::Continuation && region.bindIndex == -1) {
                    const auto &style = m_regionStyles[region.depth % m_regionStyles.size()];
                    out << style << nameStyle << label << regionStyle << "\x1b[K\n";
                }
            }
        }

        // display graph
        if (!m_regionGraph.empty()) {
            auto first = m_regionGraph.rbegin();
            auto last = m_regionGraph.rend();
            --last;
            for (; first != last; ++first) {
                out << first->labelLine << "\x1b[K\n" << first->graphLine << "\x1b[K\n";
            }
            out << first->labelLine << "\x1b[K\n" << first->graphLine << "\x1b[K\x1b[0m\n";
        }

        // keep regions that are not closed
        m_regions.erase(std::remove_if(m_regions.begin(),
                                       m_regions.end(),
                                       [](Region const &region) {
                                           return region.bindIndex != -1 || region.state == Region::State::Close;
                                       }),
                        m_regions.end());
        // all remaining regions become Continuation
        for (auto &region : m_regions) {
            region.offset = 0;
            region.state = Region::State::Continuation;
        }
    }

    struct HighlightFragment {
        QString name;
        int offset;
        int length;
        quint16 formatId;
    };

    struct ContextCaptureHighlighter : KSyntaxHighlighting::AbstractHighlighter {
        int offset;
        int length;
        int offsetNext;
        int lengthNext;

        void applyFormat(int offset, int length, const KSyntaxHighlighting::Format & /*format*/) override
        {
            offset = offsetNext;
            length = lengthNext;
            offsetNext = offset;
            lengthNext = length;
        }

        using KSyntaxHighlighting::AbstractHighlighter::highlightLine;
    };

    struct Region {
        enum class State : int8_t {
            Open,
            Close,
            Continuation,
        };

        int depth;
        int offset;
        int bindIndex;
        quint16 regionId;
        State state;
    };

    bool m_hasFormatTrace;
    bool m_hasRegionTrace;
    bool m_hasStackSizeTrace;
    bool m_hasContextTrace;

    std::vector<HighlightFragment> m_highlightedFragments;
    std::vector<GraphLine> m_formatGraph;
    ContextCaptureHighlighter m_contextCapture;

    int m_regionDepth = 0;
    std::vector<Region> m_regions;
    std::vector<GraphLine> m_regionGraph;
    std::vector<QStringView> m_regionStyles;

    QHash<const Context *, const DefinitionData *> m_defDataBycontexts;
};
} // anonymous namespace

class KSyntaxHighlighting::AnsiHighlighterPrivate
{
public:
    QTextStream out;
    QFile file;
    QStringView currentLine;
    // pairs of startColor / resetColor
    std::vector<QPair<QString, QString>> ansiStyles;
};

AnsiHighlighter::AnsiHighlighter()
    : d(new AnsiHighlighterPrivate())
{
}

AnsiHighlighter::~AnsiHighlighter() = default;

void AnsiHighlighter::setOutputFile(const QString &fileName)
{
    if (d->file.isOpen()) {
        d->file.close();
    }
    d->file.setFileName(fileName);
    if (!d->file.open(QFile::WriteOnly | QFile::Truncate)) {
        qCWarning(Log) << "Failed to open output file" << fileName << ":" << d->file.errorString();
        return;
    }
    d->out.setDevice(&d->file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d->out.setCodec("UTF-8");
#endif
}

void AnsiHighlighter::setOutputFile(FILE *fileHandle)
{
    d->file.open(fileHandle, QIODevice::WriteOnly);
    d->out.setDevice(&d->file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d->out.setCodec("UTF-8");
#endif
}

void AnsiHighlighter::highlightFile(const QString &fileName, AnsiFormat format, bool useEditorBackground, TraceOptions traceOptions)
{
    QFileInfo fi(fileName);
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly)) {
        qCWarning(Log) << "Failed to open input file" << fileName << ":" << f.errorString();
        return;
    }

    highlightData(&f, format, useEditorBackground, traceOptions);
}

void AnsiHighlighter::highlightData(QIODevice *dev, AnsiFormat format, bool useEditorBackground, TraceOptions traceOptions)
{
    if (!d->out.device()) {
        qCWarning(Log) << "No output stream defined!";
        return;
    }

    const auto is256Colors = (format == AnsiFormat::XTerm256Color);
    const auto theme = this->theme();
    const auto definition = this->definition();

    auto definitions = definition.includedDefinitions();
    definitions.append(definition);

    AnsiBuffer::ColorCache colorCache;

    AnsiBuffer foregroundColorBuffer;
    AnsiBuffer backgroundColorBuffer;
    QLatin1String foregroundDefaultColor;
    QLatin1String backgroundDefaultColor;

    // https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_parameters

    if (useEditorBackground) {
        const QRgb foregroundColor = theme.textColor(Theme::Normal);
        const QRgb backgroundColor = theme.editorColor(Theme::BackgroundColor);
        foregroundColorBuffer.appendForeground(foregroundColor, is256Colors, colorCache);
        backgroundColorBuffer.append(QLatin1String("\x1b["));
        backgroundColorBuffer.appendBackground(backgroundColor, is256Colors, colorCache);
        foregroundDefaultColor = foregroundColorBuffer.latin1();
        backgroundDefaultColor = backgroundColorBuffer.latin1().mid(2);
    }

    // ansiStyles must not be empty for applyFormat to work even with a definition without any context
    if (d->ansiStyles.empty()) {
        d->ansiStyles.resize(32);
    } else {
        d->ansiStyles[0].first.clear();
        d->ansiStyles[0].second.clear();
    }

    // initialize ansiStyles
    for (auto &&definition : std::as_const(definitions)) {
        for (auto &&format : std::as_const(DefinitionData::get(definition)->formats)) {
            const auto id = format.id();
            if (id >= d->ansiStyles.size()) {
                // better than id + 1 to avoid successive allocations
                d->ansiStyles.resize(id * 2);
            }

            AnsiBuffer buffer;

            buffer.append(QLatin1String("\x1b["));

            const bool hasFg = format.hasTextColor(theme);
            const bool hasBg = format.hasBackgroundColor(theme);
            const bool hasBold = format.isBold(theme);
            const bool hasItalic = format.isItalic(theme);
            const bool hasUnderline = format.isUnderline(theme);
            const bool hasStrikeThrough = format.isStrikeThrough(theme);

            if (hasFg) {
                buffer.appendForeground(format.textColor(theme).rgb(), is256Colors, colorCache);
            } else {
                buffer.append(foregroundDefaultColor);
            }
            if (hasBg) {
                buffer.appendBackground(format.backgroundColor(theme).rgb(), is256Colors, colorCache);
            }
            if (hasBold) {
                buffer.append(QLatin1String("1;"));
            }
            if (hasItalic) {
                buffer.append(QLatin1String("3;"));
            }
            if (hasUnderline) {
                buffer.append(QLatin1String("4;"));
            }
            if (hasStrikeThrough) {
                buffer.append(QLatin1String("9;"));
            }

            // if there is ANSI style
            if (buffer.latin1().size() > 2) {
                buffer.setFinalStyle();
                d->ansiStyles[id].first = buffer.latin1();

                if (useEditorBackground) {
                    buffer.clear();
                    const bool hasEffect = hasBold || hasItalic || hasUnderline || hasStrikeThrough;
                    if (hasBg) {
                        buffer.append(hasEffect ? QLatin1String("\x1b[0;") : QLatin1String("\x1b["));
                        buffer.append(backgroundDefaultColor);
                        buffer.setFinalStyle();
                        d->ansiStyles[id].second = buffer.latin1();
                    } else if (hasEffect) {
                        buffer.append(QLatin1String("\x1b["));
                        if (hasBold) {
                            buffer.append(QLatin1String("21;"));
                        }
                        if (hasItalic) {
                            buffer.append(QLatin1String("23;"));
                        }
                        if (hasUnderline) {
                            buffer.append(QLatin1String("24;"));
                        }
                        if (hasStrikeThrough) {
                            buffer.append(QLatin1String("29;"));
                        }
                        buffer.setFinalStyle();
                        d->ansiStyles[id].second = buffer.latin1();
                    }
                } else {
                    d->ansiStyles[id].second = QStringLiteral("\x1b[0m");
                }
            }
        }
    }

    if (useEditorBackground) {
        backgroundColorBuffer.setFinalStyle();
        backgroundDefaultColor = backgroundColorBuffer.latin1();
        d->out << backgroundDefaultColor;
    }

    QTextStream in(dev);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    if (!traceOptions) {
        State state;
        QString currentLine;
        while (in.readLineInto(&currentLine)) {
            d->currentLine = currentLine;
            state = highlightLine(d->currentLine, state);

            if (useEditorBackground) {
                d->out << QStringLiteral("\x1b[K\n");
            } else {
                d->out << QLatin1Char('\n');
            }
        }
    } else {
        AnsiBuffer buffer;
        buffer.append(QLatin1String("\x1b[0;"));
        buffer.appendBackground(theme.editorColor(useEditorBackground ? Theme::TemplateBackground : Theme::BackgroundColor), is256Colors, colorCache);
        buffer.setFinalStyle();
        DebugSyntaxHighlighter debugHighlighter;
        debugHighlighter.setDefinition(definition);
        debugHighlighter.highlightData(in, d->out, buffer.latin1(), backgroundDefaultColor, d->ansiStyles, traceOptions);
    }

    if (useEditorBackground) {
        d->out << QStringLiteral("\x1b[0m");
    }

    d->out.setDevice(nullptr);
    d->file.close();
    d->ansiStyles.clear();
}

void AnsiHighlighter::applyFormat(int offset, int length, const Format &format)
{
    auto const &ansiStyle = d->ansiStyles[format.id()];
    d->out << ansiStyle.first << d->currentLine.mid(offset, length) << ansiStyle.second;
}
