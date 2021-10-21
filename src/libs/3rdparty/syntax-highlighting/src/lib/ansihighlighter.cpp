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
#include <QMap>
#include <QTextStream>

#include <cmath>
#include <vector>

using namespace KSyntaxHighlighting;

namespace
{
struct CieLab {
    double l;
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

    // xterm color represented in CIELAB (D65)
    // see rgbToLab()
    constexpr CieLab xterm240Labs[] {
        // ignore the first 16 colors as they are unpredictable (user configurable)
        // {   0.000000,    0.000000,    0.000000},
        // {  25.535531,   48.045128,   38.057321},
        // {  46.227431,  -51.698496,   49.896846},
        // {  51.868943,  -12.929464,   56.674579},
        // {  12.971967,   47.502281,  -64.702162},
        // {  29.784667,   58.927896,  -36.487077},
        // {  48.254093,  -28.846304,   -8.476886},
        // {  77.704367,   -0.000013,    0.000005},
        // {  53.585016,   -0.000010,    0.000004},
        // {  53.240794,   80.092460,   67.203197},
        // {  87.734722,  -86.182716,   83.179321},
        // {  97.139267,  -21.553748,   94.477975},
        // {  32.297011,   79.187520, -107.860162},
        // {  60.324212,   98.234312,  -60.824892},
        // {  91.113220,  -48.087528,  -14.131186},
        // { 100.000004,   -0.000017,    0.000007},
        // 240 colors mode
        {   0.000000,    0.000000,    0.000000},
        {   7.460661,   38.391030,  -52.344094},
        {  14.108800,   49.366227,  -67.241015},
        {  20.416780,   59.708756,  -81.328423},
        {  26.461219,   69.619186,  -94.827275},
        {  32.297011,   79.187520, -107.860162},
        {  34.362921,  -41.841471,   40.383330},
        {  36.003172,  -23.346362,   -6.860652},
        {  37.721074,   -8.280292,  -28.838129},
        {  40.044712,    8.050351,  -49.077929},
        {  42.896244,   24.232072,  -67.665859},
        {  46.179103,   39.611555,  -84.835619},
        {  48.669178,  -53.727096,   51.854752},
        {  49.680825,  -41.468213,   12.871276},
        {  50.775364,  -29.978206,   -8.809511},
        {  52.309747,  -16.087685,  -29.668380},
        {  54.271652,   -0.984531,  -49.346593},
        {  56.628677,   14.436593,  -67.825764},
        {  62.217771,  -64.983255,   62.718643},
        {  62.913963,  -56.274791,   30.552786},
        {  63.677487,  -47.533738,    9.989760},
        {  64.765216,  -36.258826,  -10.655158},
        {  66.184274,  -23.179986,  -30.659176},
        {  67.928678,   -9.021871,  -49.792238},
        {  75.200318,  -75.769144,   73.128652},
        {  75.714081,  -69.238116,   46.415771},
        {  76.281325,  -62.437099,   27.358874},
        {  77.096125,  -53.317791,    7.414754},
        {  78.170587,  -42.277048,  -12.423696},
        {  79.508487,  -29.803889,  -31.743841},
        {  87.734722,  -86.182716,   83.179321},
        {  88.132543,  -81.079314,   60.784276},
        {  88.573418,  -75.649889,   43.369240},
        {  89.209664,  -68.192330,   24.408752},
        {  90.053903,  -58.903863,    5.054882},
        {  91.113220,  -48.087528,  -14.131186},
        {  17.616214,   38.884668,   27.208161},
        {  21.055194,   47.692487,  -29.530317},
        {  24.265489,   55.109279,  -50.109929},
        {  28.188460,   63.497258,  -68.189398},
        {  32.565034,   72.278448,  -84.495140},
        {  37.209055,   81.157734,  -99.539334},
        {  38.928802,  -10.464285,   45.868796},
        {  40.317682,   -0.000008,    0.000003},
        {  41.792415,    9.716881,  -22.184768},
        {  43.816568,   21.358548,  -42.829511},
        {  46.341283,   33.910621,  -61.915173},
        {  49.295490,   46.651030,  -79.609352},
        {  51.565360,  -31.106941,   55.362293},
        {  52.493892,  -22.366057,   17.186391},
        {  53.502318,  -13.755714,   -4.459620},
        {  54.922246,   -2.860324,  -25.412901},
        {  56.747662,    9.522785,  -45.263794},
        {  58.953975,   22.669708,  -63.961919},
        {  64.235031,  -48.203292,   65.170137},
        {  64.897084,  -41.171043,   33.487406},
        {  65.624132,  -33.963343,   13.012989},
        {  66.661569,  -24.464553,   -7.626328},
        {  68.017831,  -13.189300,  -27.680081},
        {  69.689139,   -0.708182,  -46.900093},
        {  76.698001,  -62.880681,   74.951857},
        {  77.195429,  -57.221495,   48.537298},
        {  77.744943,  -51.270878,   29.570908},
        {  78.534816,  -43.206116,    9.664419},
        {  79.577356,  -33.321295,  -10.175418},
        {  80.876952,  -22.010117,  -29.524777},
        {  88.898351,  -75.968373,   84.597226},
        {  89.287443,  -71.354717,   62.392493},
        {  89.718758,  -66.422134,   45.055576},
        {  90.341414,  -59.608536,   26.140485},
        {  91.167986,  -51.063898,    6.804468},
        {  92.205709,  -41.038767,  -12.384583},
        {  27.165347,   49.930374,   40.136706},
        {  29.358410,   55.725044,  -15.903001},
        {  31.581214,   61.240172,  -37.918796},
        {  34.491549,   68.043425,  -57.611837},
        {  37.945003,   75.652936,  -75.432962},
        {  41.798486,   83.706896,  -91.791834},
        {  43.266004,    9.134592,   50.930049},
        {  44.465039,   16.311047,    6.512751},
        {  45.750668,   23.372978,  -15.766712},
        {  47.534439,   32.300943,  -36.702982},
        {  49.787278,   42.444465,  -56.184495},
        {  52.457917,   53.224022,  -74.320646},
        {  54.532058,  -13.436804,   58.898437},
        {  55.385516,   -6.768114,   21.580884},
        {  56.315467,   -0.000010,    0.000004},
        {  57.630008,    8.825705,  -21.021347},
        {  59.328134,   19.179536,  -41.022229},
        {  61.391858,   30.508200,  -59.920728},
        {  66.374922,  -33.335627,   67.745825},
        {  67.003415,  -27.527170,   36.582861},
        {  67.694487,  -21.482419,   16.212526},
        {  68.682127,  -13.384693,   -4.410654},
        {  69.975892,   -3.594073,  -24.507224},
        {  71.574010,    7.447858,  -43.809975},
        {  78.315904,  -50.585277,   76.909139},
        {  78.796543,  -45.651434,   50.818542},
        {  79.327805,  -40.421790,   31.953767},
        {  80.091978,  -33.269832,   12.092110},
        {  81.101528,  -24.409844,   -7.745063},
        {  82.361425,  -14.154994,  -27.121909},
        {  90.168532,  -65.770182,   86.138290},
        {  90.548420,  -61.599052,   64.141611},
        {  90.969646,  -57.119911,   46.891522},
        {  91.577948,  -50.900805,   28.027875},
        {  92.385840,  -43.052445,    8.713289},
        {  93.400696,  -33.779293,  -10.477090},
        {  36.208754,   60.391097,   50.573778},
        {  37.739975,   64.495259,   -2.438323},
        {  39.353431,   68.650313,  -25.128730},
        {  41.549773,   74.070366,  -45.863018},
        {  44.264011,   80.458448,  -64.848646},
        {  47.410429,   87.520359,  -82.356598},
        {  48.637025,   27.330267,   57.029239},
        {  49.649655,   32.345900,   14.536338},
        {  50.745209,   37.483199,   -7.743369},
        {  52.280931,   44.249599,  -28.930913},
        {  54.244425,   52.280310,  -48.806029},
        {  56.603189,   61.178271,  -67.411894},
        {  58.455996,    5.073270,   63.495100},
        {  59.223239,   10.069966,   27.347971},
        {  60.062286,   15.267320,    5.894811},
        {  61.253487,   22.225763,  -15.176012},
        {  62.800706,   30.633519,  -35.336720},
        {  64.692889,   40.111206,  -54.465103},
        {  69.308960,  -16.251898,   71.238024},
        {  69.895406,  -11.599340,   40.796840},
        {  70.541246,   -6.687200,   20.584981},
        {  71.466005,   -0.000013,    0.000005},
        {  72.680407,    8.238495,  -20.139565},
        {  74.184959,   17.716313,  -39.540668},
        {  80.579920,  -35.513903,   79.627400},
        {  81.038448,  -31.346986,   53.992269},
        {  81.545637,  -26.892900,   35.276028},
        {  82.275842,  -20.742163,   15.484099},
        {  83.241675,  -13.032556,   -4.342368},
        {  84.448794,   -3.993322,  -23.750841},
        {  91.967824,  -52.701251,   88.309654},
        {  92.335220,  -49.036719,   66.608006},
        {  92.742744,  -45.081868,   49.483561},
        {  93.331530,  -39.558175,   30.696050},
        {  94.113989,  -32.535991,   11.415203},
        {  95.097663,  -24.169464,   -7.773705},
        {  44.874337,   70.414781,   59.082945},
        {  46.012582,   73.488282,   10.528988},
        {  47.236695,   76.706186,  -12.348562},
        {  48.940884,   81.051413,  -33.681818},
        {  51.101856,   86.364529,  -53.475339},
        {  53.674597,   92.446330,  -71.879038},
        {  54.695304,   43.548940,   63.726908},
        {  55.544895,   47.195327,   23.494868},
        {  56.470786,   51.029166,    1.345906},
        {  57.779848,   56.225393,  -20.000213},
        {  59.471313,   62.597129,  -40.204567},
        {  61.527524,   69.897355,  -59.241373},
        {  63.159654,   22.859865,   68.897396},
        {  63.839588,   26.634208,   34.185583},
        {  64.585756,   30.632858,   12.941230},
        {  65.649581,   36.098152,   -8.134153},
        {  67.038832,   42.864229,  -28.433915},
        {  68.748596,   50.691293,  -47.788927},
        {  72.964214,    1.430076,   75.529188},
        {  73.503895,    5.119471,   45.996429},
        {  74.099224,    9.062101,   26.005435},
        {  74.953413,   14.504783,    5.492273},
        {  76.078172,   21.324042,  -14.677149},
        {  77.476211,   29.315742,  -34.177868},
        {  83.468499,  -18.949380,   83.062075},
        {  83.900955,  -15.492587,   58.010021},
        {  84.379703,  -11.768724,   39.493278},
        {  85.069678,   -6.579078,   19.801555},
        {  85.983569,   -0.000015,    0.000006},
        {  87.127728,    7.813055,  -19.437771},
        {  94.298345,  -37.668200,   91.102418},
        {  94.650453,  -34.512373,   69.782914},
        {  95.041192,  -31.089517,   52.825519},
        {  95.606040,  -26.280220,   34.142079},
        {  96.357251,  -20.119939,   14.910613},
        {  97.302528,  -12.715984,   -4.270744},
        {  53.240794,   80.092460,   67.203197},
        {  54.125781,   82.492192,   22.910970},
        {  55.088767,   85.054618,    0.168144},
        {  56.447798,   88.591017,  -21.450672},
        {  58.199846,   93.025112,  -41.765998},
        {  60.324212,   98.234312,  -60.824892},
        {  61.177753,   58.007184,   70.725237},
        {  61.892577,   60.769076,   32.940064},
        {  62.675958,   63.722867,   11.059157},
        {  63.790979,   67.805180,  -10.333124},
        {  65.243976,   72.929281,  -30.773134},
        {  67.027700,   78.950491,  -50.165199},
        {  68.456202,   39.347025,   74.858462},
        {  69.054426,   42.256401,   41.778310},
        {  69.712953,   45.379691,   20.832601},
        {  70.655381,   49.714784,   -0.184657},
        {  71.892132,   55.183573,  -20.579895},
        {  73.423104,   61.643523,  -40.132156},
        {  77.236080,   18.715563,   80.467683},
        {  77.727829,   21.651859,   52.000981},
        {  78.271171,   24.819797,   32.297655},
        {  79.052359,   29.242703,   11.899654},
        {  80.083760,   34.862654,   -8.273975},
        {  81.369962,   41.554730,  -27.861347},
        {  86.930570,   -1.923749,   87.132036},
        {  87.334594,    0.925621,   62.778902},
        {  87.782260,    4.015632,   44.514668},
        {  88.428154,    8.356443,   24.958589},
        {  89.284922,   13.915222,    5.202570},
        {  90.359536,   20.594187,  -14.254942},
        {  97.139267,  -21.553748,   94.477975},
        {  97.473993,  -18.866927,   73.623332},
        {  97.845623,  -15.939407,   56.875584},
        {  98.383182,  -11.803189,   38.326889},
        {  99.098697,   -6.467219,   19.163906},
        { 100.000004,   -0.000017,    0.000007},
        {   2.193399,   -0.000001,    0.000000},
        {   5.463889,   -0.000002,    0.000001},
        {  10.268185,   -0.000004,    0.000002},
        {  15.159721,   -0.000004,    0.000002},
        {  19.865535,   -0.000005,    0.000002},
        {  24.421321,   -0.000006,    0.000002},
        {  28.851904,   -0.000006,    0.000003},
        {  33.175474,   -0.000007,    0.000003},
        {  37.405892,   -0.000008,    0.000003},
        {  41.554045,   -0.000008,    0.000003},
        {  45.628691,   -0.000009,    0.000004},
        {  49.637017,   -0.000009,    0.000004},
        {  53.585016,   -0.000010,    0.000004},
        {  57.477759,   -0.000011,    0.000004},
        {  61.319585,   -0.000011,    0.000004},
        {  65.114248,   -0.000012,    0.000005},
        {  68.865021,   -0.000012,    0.000005},
        {  72.574786,   -0.000013,    0.000005},
        {  76.246094,   -0.000013,    0.000005},
        {  79.881220,   -0.000014,    0.000006},
        {  83.482203,   -0.000014,    0.000006},
        {  87.050883,   -0.000015,    0.000006},
        {  90.588923,   -0.000015,    0.000006},
        {  94.097838,   -0.000016,    0.000006},
    };

    // http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    constexpr double sRGB_D65[] {
        0.4124564, 0.3575761, 0.1804375,
        0.2126729, 0.7151522, 0.0721750,
        0.0193339, 0.1191920, 0.9503041,
    };

    // http://www.brucelindbloom.com/index.html?WorkingSpaceInfo.html
    constexpr double illuminant_D65[] {
        0.95047, 1.00000, 1.08883,
    };
// clang-format on

// convert a sRGB (D65) color to CIELAB (D65)
CieLab rgbToLab(QRgb rgb)
{
    // Perform the inverse gamma companding for a sRGB color
    // http://www.brucelindbloom.com/index.html?Eqn_RGB_to_XYZ.html
    auto inverseGammaCompanding = [](int c) {
        if (c <= 10) {
            return c / (255.0 * 12.92);
        } else {
            return std::pow((c / 255.0 + 0.055) / 1.055, 2.4);
        }
    };

    const double r = inverseGammaCompanding(qRed(rgb));
    const double g = inverseGammaCompanding(qGreen(rgb));
    const double b = inverseGammaCompanding(qBlue(rgb));

    const double x = (r * sRGB_D65[0] + g * sRGB_D65[1] + b * sRGB_D65[2]);
    const double y = (r * sRGB_D65[3] + g * sRGB_D65[4] + b * sRGB_D65[5]);
    const double z = (r * sRGB_D65[6] + g * sRGB_D65[7] + b * sRGB_D65[8]);

    // http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_Lab.html
    auto f = [](double t) {
        if (t > 216.0 / 24389.0) {
            return std::cbrt(t);
        } else {
            return t * (24389.0 / (27.0 * 116.0)) + 4.0 / 29.0;
        }
    };

    const double f_x = f(x / illuminant_D65[0]);
    const double f_y = f(y / illuminant_D65[1]);
    const double f_z = f(z / illuminant_D65[2]);

    return CieLab{
        116.0 * f_y - 16.0,
        500.0 * (f_x - f_y),
        200.0 * (f_y - f_z),
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
double calculate_CIEDE2000(const CieLab &color1, const CieLab &color2)
{
    const double L1 = color1.l;
    const double a1 = color1.a;
    const double b1 = color1.b;
    const double L2 = color2.l;
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
    using ColorCache = QMap<QRgb, int>;

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
                const auto lab = rgbToLab(rgb);
                // find the nearest xterm color
                for (CieLab const &xtermLab : xterm240Labs) {
                    auto dist2 = calculate_CIEDE2000(lab, xtermLab);
                    if (dist2 < dist) {
                        dist = dist2;
                        idx = &xtermLab - xterm240Labs;
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

void fillString(QString &s, int n, const QString &fill)
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

    template<class String> void pushLabel(int offset, String const &s, int charCounter)
    {
        Q_ASSERT(offset >= labelLineLength);
        const int n = offset - labelLineLength;
        labelLineLength += charCounter + n;
        fillLine(labelLine, n);
        labelLine += s;
        nextLabelOffset = labelLineLength;
    }

    template<class String> void pushGraph(int offset, String const &s, int charCounter)
    {
        Q_ASSERT(offset >= graphLineLength);
        const int n = offset - graphLineLength;
        graphLineLength += charCounter + n;
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
        m_defData = DefinitionData::get(def);
        m_contextCapture.setDefinition(def);
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
        while (!in.atEnd()) {
            const QString currentLine = in.readLine();
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
    void appendContextNames(State &state, const QString &currentLine)
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
            const auto defData = DefinitionData::get(context->definition());
            const auto contextName = (defData != m_defData) ? QString(QLatin1Char('<') % defData->name % QLatin1Char('>')) : QString();
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
    DefinitionData *m_defData;

    int m_regionDepth = 0;
    std::vector<Region> m_regions;
    std::vector<GraphLine> m_regionGraph;
    std::vector<QStringView> m_regionStyles;
};
} // anonymous namespace

class KSyntaxHighlighting::AnsiHighlighterPrivate
{
public:
    QTextStream out;
    QFile file;
    QString currentLine;
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

    // initialize ansiStyles
    for (auto &&definition : std::as_const(definitions)) {
        const auto formats = definition.formats();
        for (auto &&format : formats) {
            const auto id = format.id();
            if (id >= d->ansiStyles.size()) {
                // better than id + 1 to avoid successive allocations
                d->ansiStyles.resize(std::max(std::size_t(id * 2), std::size_t(32)));
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
        while (!in.atEnd()) {
            d->currentLine = in.readLine();
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
    d->out << ansiStyle.first << QStringView(d->currentLine).mid(offset, length) << ansiStyle.second;
}
