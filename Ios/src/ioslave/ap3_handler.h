/**
 * This is an easy way to return into the Ios.cpp
 * Alternative would be to set this in variant.h, but
 * this is better. You do not need to define this for ALL boards
 *
 * version 1.0 / April 2023 / paulvha
 */

extern void am_ios_handler(void)          __attribute ((weak));
#define IOS_IT_HANDLER am_ios_handler
