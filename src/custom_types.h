#pragma once

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef int i32;
typedef long long i64;

typedef float f32;
typedef double f64;

static_assert(sizeof(u16) == 2);

static_assert(sizeof(u32) == 4);
static_assert(sizeof(i32) == 4);
static_assert(sizeof(f32) == 4);

static_assert(sizeof(u64) == 8);
static_assert(sizeof(i64) == 8);
static_assert(sizeof(f64) == 8);
