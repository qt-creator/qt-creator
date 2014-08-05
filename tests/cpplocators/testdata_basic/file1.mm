// Copyright header to keep the Qt Insanity Bot happy.

@protocol NSObject
@end

@interface NSObject<NSObject>
@end

@protocol MyProtocol <NSObject>
- (void) someMethod;
@end

@interface MyClass: NSObject <MyProtocol>
@end

@implementation MyClass
- (void) someMethod {}
@end

@implementation MyClass(MyCategory)
- (void) anotherMethod;{}
- (void) anotherMethod:(NSObject*)withAnObject{}
@end
