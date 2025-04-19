#pragma once
// allows the use of libm sqrt
int __errno;

extern "C" int *__errno(void);