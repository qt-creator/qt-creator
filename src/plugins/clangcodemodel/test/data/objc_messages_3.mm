// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

@protocol Aaaa
@end

@interface Bbbb
@end

@interface PopCornTracker {
    int _quality;
    int _eatenAmount;
    int _remainedAmount;
}
- (int) eatenAmount;
- (int) spectacleQuality;
- (int) desiredAmountForDramaDose: (int)dose andPersonsCount: (int) count;
+ (id) createNewTracker;
+ (id) createOldTracker:(Bbbb<Aaaa> *) aabb;
- (id) initWithOldTracker:(Bbbb<Aaaa> *) aabb;
@end

@interface AdvancedPopCornTracker : PopCornTracker {
}

- <<<<

@end
