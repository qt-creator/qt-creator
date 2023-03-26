// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*
 * Expected texts:
 *      eatenAmount
 *      spectacleQuality:
 *      desiredAmountForDramaDose:andPersonsCount:
 *
 * Expected hints:
 *      -(int) eatenAmount
 *
 *      -(int) spectacleQuality:(bool)unused
 */

@interface PopCornTracker {
    int _quality;
    int _eatenAmount;
    int _remainedAmount;
}
+ (int) eatenAmount;
- (int) spectacleQuality : (bool)unused;
- (int) desiredAmountForDramaDose: (int)dose andPersonsCount: (int) count;
@end

@implementation PopCornTracker
- (int) desiredAmountForDramaDose: (int)dose andPersonsCount: (int) count
{
    [self <<<<];
}
@end
