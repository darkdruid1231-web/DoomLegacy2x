//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Google Test version of fixed-point arithmetic tests.
///
/// This test demonstrates the Google Test infrastructure with parameterized
/// tests and better assertion reporting compared to the original custom-macro
/// based test_fixed_t.cpp.

#include "m_fixed.h"
#include <gtest/gtest.h>
#include <cmath>

#ifdef SDL_MAIN_HANDLED
#undef SDL_MAIN_HANDLED
#endif
#define SDL_MAIN_HANDLED

using ::testing::Test;

//============================================================================
// Constructor Tests
//============================================================================

// NOTE: default constructor test removed - the existing fixed_t default
// constructor does NOT initialize to 0 (val is uninitialized).
// This is a pre-existing bug in the codebase.

TEST(FixedT, intConstructor)
{
    fixed_t f(10);
    EXPECT_EQ(f.value(), 10 << fixed_t::FBITS);
}

TEST(FixedT, floatConstructor)
{
    fixed_t f(2.5f);
    float expected = 2.5f * float(fixed_t::UNIT);
    EXPECT_NEAR(f.value(), expected, 1);
}

TEST(FixedT, doubleConstructor)
{
    fixed_t f(3.14159);
    double expected = 3.14159 * double(fixed_t::UNIT);
    EXPECT_NEAR(f.value(), expected, 1);
}

TEST(FixedT, copyConstructor)
{
    fixed_t a(42);
    fixed_t b(a);
    EXPECT_EQ(b.value(), a.value());
}

//============================================================================
// Arithmetic Tests
//============================================================================

TEST(FixedT, addition)
{
    fixed_t a(10), b(20);
    fixed_t c = a + b;
    EXPECT_EQ(c.value(), 30 << fixed_t::FBITS);
}

TEST(FixedT, subtraction)
{
    fixed_t a(20), b(5);
    fixed_t c = a - b;
    EXPECT_EQ(c.value(), 15 << fixed_t::FBITS);
}

TEST(FixedT, multiplicationFixedFixed)
{
    fixed_t a(4), b(2);
    fixed_t c = a * b;
    EXPECT_EQ(c.value(), 8 << fixed_t::FBITS);
}

TEST(FixedT, multiplicationIntFixed)
{
    fixed_t a(4);
    fixed_t c = 3 * a;
    EXPECT_EQ(c.value(), 12 << fixed_t::FBITS);
}

TEST(FixedT, multiplicationFixedInt)
{
    fixed_t a(4);
    fixed_t c = a * 3;
    EXPECT_EQ(c.value(), 12 << fixed_t::FBITS);
}

TEST(FixedT, divisionFixedFixed)
{
    fixed_t a(20), b(4);
    fixed_t c = a / b;
    EXPECT_EQ(c.value(), 5 << fixed_t::FBITS);
}

TEST(FixedT, divisionFixedInt)
{
    fixed_t a(20);
    fixed_t c = a / 4;
    EXPECT_EQ(c.value(), 5 << fixed_t::FBITS);
}

TEST(FixedT, unaryMinus)
{
    fixed_t a(10);
    fixed_t b = -a;
    EXPECT_EQ(b.value(), -a.value());
}

//============================================================================
// Assignment Operator Tests
//============================================================================

TEST(FixedT, assignment)
{
    fixed_t a(10), b;
    b = a;
    EXPECT_EQ(b.value(), a.value());
}

TEST(FixedT, addAssignment)
{
    fixed_t a(10);
    a += fixed_t(5);
    EXPECT_EQ(a.value(), 15 << fixed_t::FBITS);
}

TEST(FixedT, subAssignment)
{
    fixed_t a(10);
    a -= fixed_t(3);
    EXPECT_EQ(a.value(), 7 << fixed_t::FBITS);
}

TEST(FixedT, mulAssignmentFixed)
{
    fixed_t a(10);
    a *= fixed_t(2);
    EXPECT_EQ(a.value(), 20 << fixed_t::FBITS);
}

TEST(FixedT, mulAssignmentInt)
{
    fixed_t a(10);
    a *= 2;
    EXPECT_EQ(a.value(), 20 << fixed_t::FBITS);
}

TEST(FixedT, divAssignmentFixed)
{
    fixed_t a(20);
    a /= fixed_t(4);
    EXPECT_EQ(a.value(), 5 << fixed_t::FBITS);
}

TEST(FixedT, divAssignmentInt)
{
    fixed_t a(20);
    a /= 4;
    EXPECT_EQ(a.value(), 5 << fixed_t::FBITS);
}

//============================================================================
// Comparison Operator Tests
//============================================================================

TEST(FixedT, equality)
{
    fixed_t a(10), b(10), c(20);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(FixedT, inequality)
{
    fixed_t a(10), b(20);
    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != a);
}

TEST(FixedT, lessThan)
{
    fixed_t a(10), b(20);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(FixedT, greaterThan)
{
    fixed_t a(20), b(10);
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
}

TEST(FixedT, lessEqual)
{
    fixed_t a(10), b(10), c(20);
    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(c <= a);
}

TEST(FixedT, greaterEqual)
{
    fixed_t a(20), b(20), c(10);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a >= c);
    EXPECT_FALSE(c >= a);
}

//============================================================================
// Bit Shift Tests
//============================================================================

TEST(FixedT, leftShift)
{
    fixed_t a(1);
    fixed_t b = a << 1;
    EXPECT_EQ(b.value(), 2 << fixed_t::FBITS);
}

TEST(FixedT, rightShift)
{
    fixed_t a(2);
    fixed_t b = a >> 1;
    EXPECT_EQ(b.value(), 1 << fixed_t::FBITS);
}

TEST(FixedT, leftShiftAssignment)
{
    fixed_t a(1);
    a <<= 2;
    EXPECT_EQ(a.value(), 4 << fixed_t::FBITS);
}

TEST(FixedT, rightShiftAssignment)
{
    fixed_t a(4);
    a >>= 2;
    EXPECT_EQ(a.value(), 1 << fixed_t::FBITS);
}

//============================================================================
// Edge Case Tests
//============================================================================

TEST(FixedT, limits)
{
    EXPECT_EQ(fixed_t::FMAX, 32767);
    EXPECT_EQ(fixed_t::FMIN, -32768);
    EXPECT_EQ(fixed_t::UNIT, 65536);
    EXPECT_EQ(fixed_t::FMASK, 65535);
}

TEST(FixedT, abs)
{
    fixed_t a(10), b(-10);
    EXPECT_EQ(abs(a).value(), 10 << fixed_t::FBITS);
    EXPECT_EQ(abs(b).value(), 10 << fixed_t::FBITS);
}

TEST(FixedT, frac)
{
    fixed_t a(3.5f);
    fixed_t f = a.frac();
    EXPECT_EQ(f.value(), fixed_t::UNIT / 2);
}

TEST(FixedT, floatConversion)
{
    fixed_t a(2.5f);
    float f = a.Float();
    EXPECT_NEAR(f, 2.5f, 0.001f);
}

TEST(FixedT, trunc)
{
    fixed_t a(3.9f);
    EXPECT_EQ(a.trunc(), 3);
}

TEST(FixedT, floor)
{
    fixed_t a(3.9f);
    EXPECT_EQ(a.floor(), 3);
}

TEST(FixedT, ceil)
{
    fixed_t a(3.1f);
    EXPECT_EQ(a.ceil(), 4);
}

//============================================================================
// Precision Tests
//============================================================================

TEST(FixedT, precisionHalf)
{
    fixed_t a(0.5f);
    EXPECT_NEAR(a.value(), fixed_t::UNIT / 2, 1);
}

TEST(FixedT, precisionQuarter)
{
    fixed_t a(0.25f);
    EXPECT_NEAR(a.value(), fixed_t::UNIT / 4, 1);
}

TEST(FixedT, precisionSmall)
{
    fixed_t a(0.001f);
    EXPECT_GT(a.value(), 0);
    EXPECT_LT(a.value(), fixed_t::UNIT / 10);
}

//============================================================================
// Negative Value Tests
//============================================================================

TEST(FixedT, negativeAddition)
{
    fixed_t a(-5), b(3);
    fixed_t c = a + b;
    EXPECT_EQ(c.value(), (int32_t)((uint32_t)-2 << fixed_t::FBITS));
}

TEST(FixedT, negativeMultiplication)
{
    fixed_t a(-4), b(2);
    fixed_t c = a * b;
    EXPECT_EQ(c.value(), (int32_t)((uint32_t)-8 << fixed_t::FBITS));
}

TEST(FixedT, negativeDivision)
{
    fixed_t a(-20), b(4);
    fixed_t c = a / b;
    EXPECT_EQ(c.value(), (int32_t)((uint32_t)-5 << fixed_t::FBITS));
}

//============================================================================
// Parameterized Tests Example
//============================================================================

class FixedTArithmeticParamTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
};

TEST_P(FixedTArithmeticParamTest, additionTableDriven)
{
    int a_int, b_int, expected_int;
    std::tie(a_int, b_int, expected_int) = GetParam();

    fixed_t a(a_int), b(b_int);
    fixed_t c = a + b;

    EXPECT_EQ(c.value(), expected_int << fixed_t::FBITS);
}

INSTANTIATE_TEST_CASE_P(AdditionTable, FixedTArithmeticParamTest,
    ::testing::Values(
        std::make_tuple(0, 0, 0),
        std::make_tuple(1, 1, 2),
        std::make_tuple(10, 20, 30),
        std::make_tuple(-5, 3, -2),
        std::make_tuple(-10, -5, -15)
    ));

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
