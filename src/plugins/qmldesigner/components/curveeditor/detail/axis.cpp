/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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
#include "axis.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <limits>

namespace DesignTools {

// The following is based on: "An Extension of Wilkinson's Algorithm for Positioning Tick Labels on Axes"
// by Justin Talbot, Sharon Lin and Pat Hanrahan.
// The Implementation uses the conventions and variable names found in the paper where:
// Inputs:
//     Q           Preference ordered list of nice steps
//     dmin/dmax   Data range
//     pt          Target label density
//     fst         Target font size (currently ignored)
//     Formats     set of label formats (currently ignored)
// Other:
//     q           Element of Q
//     i           Index of q in Q
//     j           Label skipping amount (taking only jth label into account)
//     lmin/lmax   Labeling sequence range
//     v           Indicator if labeling should include 0. (1 means yes / 0 no)

double simplicity(int i, std::vector<double> &Q, int j, int v = 1)
{
    assert(Q.size() > 1);
    return 1.0 - ((static_cast<double>(i) - 1.0) / (static_cast<double>(Q.size()) - 1.0))
           - static_cast<double>(j) + static_cast<double>(v);
}

double coverage(double dmin, double dmax, double lmin, double lmax)
{
    return 1.0 - 0.5 *
        ((std::pow(dmax - lmax, 2.0) + std::pow(dmin - lmin, 2.0)) /
         (std::pow(0.1 * (dmax - dmin), 2.0)));
}

double coverageMax(double dmin, double dmax, double labelingRange)
{
    double dataRange = dmax - dmin;
    if (labelingRange > dataRange) {
        double range = (labelingRange - dataRange) / 2.;
        return 1 - 0.5 * ((std::pow(range, 2.0) * 2.0) / (std::pow(0.1 * (dmax - dmin), 2.0)));
    }
    return 1;
}

double density(double p, double pt)
{
    return 2.0 - std::max(p / pt, pt / p);
}

double densityMax(double p, double pt)
{
    if (p >= pt)
        return 2 - p / pt;
    else
        return 1;
}

namespace Legibility {

class Format
{
public:
    double legibility() const { return 1.0; }
};

class FontSize
{
public:
    double legibility() const { return 1.0; }
};

class Orientation
{
public:
    double legibility() const { return 1.0; }
};

class Overlap
{
public:
    double legibility() const { return 1.0; }
};

} // End namespace Legibility.

using namespace Legibility;

double legibility(const Format &fmt, const FontSize &fs, const Orientation &ori, const Overlap &ovl)
{
    return (fmt.legibility() + fs.legibility() + ori.legibility() + ovl.legibility()) / 4.0;
}

struct LabelingInfo
{
    double l;
    Format lformat;
};

LabelingInfo optLegibility(int k, double lmin, double lstep)
{
    std::vector<double> stepSequence;
    for (int i = 0; i < k; ++i)
        stepSequence.push_back(lmin + i * lstep);

    Format fmt;
    FontSize fs;
    Orientation ori;
    Overlap ovl;

    LabelingInfo info;
    info.l = legibility(fmt, fs, ori, ovl);
    info.lformat = fmt;
    return info;
}

Axis Axis::compute(double dmin, double dmax, double height, double pt)
{
    Axis result = {0.0, 0.0, 0.0};

    auto score = [](double a, double b, double c, double d) {
        return a * 0.2 + b * 0.25 + c * 0.5 + d * 0.05;
    };

    std::vector<double> Q = {1., 5., 2., 2.5, 3.};
    //std::vector<double> Q = {1., 5., 2., 2.5, 3., 4., 1.5, 7., 6., 8., 9.};

    double best_score = -2.0;
    int j = 1;
    while (j < std::numeric_limits<int>::max()) {
        for (int i = 0; i < static_cast<int>(Q.size()); ++i) {
            double q = Q[i];

            auto simMax = simplicity(i, Q, j);
            if (score(simMax, 1.0, 1.0, 1.0) < best_score) {
                j = std::numeric_limits<int>::max() - 1;
                break;
            }

            int k = 2; // label count
            while (k < std::numeric_limits<int>::max()) {
                auto p = k / height;
                auto denMax = densityMax(p, pt);
                if (score(simMax, 1.0, denMax, 1.0) < best_score)
                    break;

                auto delta = (dmax - dmin) / (k + 1) / (j * q);
                int z = std::ceil(std::log10(delta));
                while (z < std::numeric_limits<int>::max()) {
                    auto lstep = q * static_cast<double>(j) * std::pow(10, z);
                    auto covMax = coverageMax(dmin, dmax, lstep * (k - 1));
                    if (score(simMax, covMax, denMax, 1.0) < best_score)
                        break;

                    int start = (std::floor(dmax / lstep) - (k - 1)) * j;
                    int end = std::ceil(dmin / lstep) * j;
                    for (; start <= end; ++start) {
                        double lmin = start * lstep / j;
                        double lmax = lmin + lstep * (k - 1);

                        double s = simplicity(i, Q, j);
                        double d = density(p, pt);
                        double c = coverage(dmin, dmax, lmin, lmax);

                        if (score(s, c, d, 1.0) < best_score)
                            continue;

                        auto info = optLegibility(k, lmin, lstep);
                        const double cscore = score(s, c, d, info.l);
                        if (cscore > best_score) {
                            best_score = cscore;
                            result = {lmin, lmax, lstep};
                        }
                    }
                    z++;
                }
                k++;
            }
        }
        j++;
    }
    return result;
}

} // End namespace DesignTools.
