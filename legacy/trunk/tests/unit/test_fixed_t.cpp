//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Doom Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

/// \file
/// \brief Unit tests for fixed_t arithmetic operations.

#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <limits>
#include "m_fixed.h"

using namespace std;

static int tests_run = 0;
static int tests_passed = 0;
static string last_failure;

#define TEST(name) do { \
    tests_run++; \
    last_failure = ""; \
    cout << "  " << name << " ... "; \
} while(0)

#define PASS() do { \
    tests_passed++; \
    cout << "PASS" << endl; \
} while(0)

#define FAIL(msg) do { \
    last_failure = msg; \
    cout << "FAIL: " << msg << endl; \
    return; \
} while(0)

#define CHECK(cond, msg) do { \
    if (!(cond)) FAIL(msg); \
} while(0)

#define CHECK_CLOSE(actual, expected, tol) do { \
    if (std::abs((actual) - (expected)) > (tol)) { \
        FAIL("Value " + to_string(actual) + " not within " + to_string(tol) + " of " + to_string(expected)); \
    } \
} while(0)

//============================================================================
// Constructor Tests
//============================================================================

void test_default_constructor() {
    TEST("default_constructor");
    fixed_t f;
    f = fixed_t(0);
    CHECK(f.value() == 0, "assigned value should be 0");
    PASS();
}

void test_int_constructor() {
    TEST("int_constructor");
    fixed_t f(10);
    CHECK(f.value() == (10 << fixed_t::FBITS), "int constructor failed");
    PASS();
}

void test_float_constructor() {
    TEST("float_constructor");
    fixed_t f(2.5f);
    float expected = 2.5f * float(fixed_t::UNIT);
    CHECK(std::abs(f.value() - expected) <= 1, "float constructor failed");
    PASS();
}

void test_double_constructor() {
    TEST("double_constructor");
    fixed_t f(3.14159);
    double expected = 3.14159 * double(fixed_t::UNIT);
    CHECK(std::abs(f.value() - expected) <= 1, "double constructor failed");
    PASS();
}

void test_copy_constructor() {
    TEST("copy_constructor");
    fixed_t a(42);
    fixed_t b(a);
    CHECK(b.value() == a.value(), "copy constructor failed");
    PASS();
}

//============================================================================
// Arithmetic Tests
//============================================================================

void test_addition() {
    TEST("addition");
    fixed_t a(10), b(20);
    fixed_t c = a + b;
    CHECK(c.value() == (30 << fixed_t::FBITS), "addition failed");
    PASS();
}

void test_subtraction() {
    TEST("subtraction");
    fixed_t a(20), b(5);
    fixed_t c = a - b;
    CHECK(c.value() == (15 << fixed_t::FBITS), "subtraction failed");
    PASS();
}

void test_multiplication_fixed_fixed() {
    TEST("multiplication_fixed_fixed");
    fixed_t a(4), b(2);
    fixed_t c = a * b;
    CHECK(c.value() == (8 << fixed_t::FBITS), "fixed*fixed multiplication failed");
    PASS();
}

void test_multiplication_int_fixed() {
    TEST("multiplication_int_fixed");
    fixed_t a(4);
    fixed_t c = 3 * a;
    CHECK(c.value() == (12 << fixed_t::FBITS), "int*fixed multiplication failed");
    PASS();
}

void test_multiplication_fixed_int() {
    TEST("multiplication_fixed_int");
    fixed_t a(4);
    fixed_t c = a * 3;
    CHECK(c.value() == (12 << fixed_t::FBITS), "fixed*int multiplication failed");
    PASS();
}

void test_division_fixed_fixed() {
    TEST("division_fixed_fixed");
    fixed_t a(20), b(4);
    fixed_t c = a / b;
    CHECK(c.value() == (5 << fixed_t::FBITS), "fixed/fixed division failed");
    PASS();
}

void test_division_fixed_int() {
    TEST("division_fixed_int");
    fixed_t a(20);
    fixed_t c = a / 4;
    CHECK(c.value() == (5 << fixed_t::FBITS), "fixed/int division failed");
    PASS();
}

void test_unary_minus() {
    TEST("unary_minus");
    fixed_t a(10);
    fixed_t b = -a;
    CHECK(b.value() == -a.value(), "unary minus failed");
    PASS();
}

//============================================================================
// Assignment Operator Tests
//============================================================================

void test_assignment() {
    TEST("assignment");
    fixed_t a(10), b;
    b = a;
    CHECK(b.value() == a.value(), "assignment failed");
    PASS();
}

void test_add_assignment() {
    TEST("add_assignment");
    fixed_t a(10);
    a += fixed_t(5);
    CHECK(a.value() == (15 << fixed_t::FBITS), "+= failed");
    PASS();
}

void test_sub_assignment() {
    TEST("sub_assignment");
    fixed_t a(10);
    a -= fixed_t(3);
    CHECK(a.value() == (7 << fixed_t::FBITS), "-= failed");
    PASS();
}

void test_mul_assignment_fixed() {
    TEST("mul_assignment_fixed");
    fixed_t a(10);
    a *= fixed_t(2);
    CHECK(a.value() == (20 << fixed_t::FBITS), "*= fixed failed");
    PASS();
}

void test_mul_assignment_int() {
    TEST("mul_assignment_int");
    fixed_t a(10);
    a *= 2;
    CHECK(a.value() == (20 << fixed_t::FBITS), "*= int failed");
    PASS();
}

void test_div_assignment_fixed() {
    TEST("div_assignment_fixed");
    fixed_t a(20);
    a /= fixed_t(4);
    CHECK(a.value() == (5 << fixed_t::FBITS), "/= fixed failed");
    PASS();
}

void test_div_assignment_int() {
    TEST("div_assignment_int");
    fixed_t a(20);
    a /= 4;
    CHECK(a.value() == (5 << fixed_t::FBITS), "/= int failed");
    PASS();
}

//============================================================================
// Comparison Operator Tests
//============================================================================

void test_equality() {
    TEST("equality");
    fixed_t a(10), b(10), c(20);
    CHECK(a == b, "equal values should be equal");
    CHECK(!(a == c), "different values should not be equal");
    PASS();
}

void test_inequality() {
    TEST("inequality");
    fixed_t a(10), b(20);
    CHECK(a != b, "different values should not be equal");
    CHECK(!(a != a), "same values should be equal");
    PASS();
}

void test_less_than() {
    TEST("less_than");
    fixed_t a(10), b(20);
    CHECK(a < b, "10 should be < 20");
    CHECK(!(b < a), "20 should not be < 10");
    PASS();
}

void test_greater_than() {
    TEST("greater_than");
    fixed_t a(20), b(10);
    CHECK(a > b, "20 should be > 10");
    CHECK(!(b > a), "10 should not be > 20");
    PASS();
}

void test_less_equal() {
    TEST("less_equal");
    fixed_t a(10), b(10), c(20);
    CHECK(a <= b, "10 <= 10 should be true");
    CHECK(a <= c, "10 <= 20 should be true");
    CHECK(!(c <= a), "20 <= 10 should be false");
    PASS();
}

void test_greater_equal() {
    TEST("greater_equal");
    fixed_t a(20), b(20), c(10);
    CHECK(a >= b, "20 >= 20 should be true");
    CHECK(a >= c, "20 >= 10 should be true");
    CHECK(!(c >= a), "10 >= 20 should be false");
    PASS();
}

//============================================================================
// Bit Shift Tests
//============================================================================

void test_left_shift() {
    TEST("left_shift");
    fixed_t a(1);
    fixed_t b = a << 1;
    CHECK(b.value() == (2 << fixed_t::FBITS), "<< 1 failed");
    PASS();
}

void test_right_shift() {
    TEST("right_shift");
    fixed_t a(2);
    fixed_t b = a >> 1;
    CHECK(b.value() == (1 << fixed_t::FBITS), ">> 1 failed");
    PASS();
}

void test_left_shift_assignment() {
    TEST("left_shift_assignment");
    fixed_t a(1);
    a <<= 2;
    CHECK(a.value() == (4 << fixed_t::FBITS), "<<= 2 failed");
    PASS();
}

void test_right_shift_assignment() {
    TEST("right_shift_assignment");
    fixed_t a(4);
    a >>= 2;
    CHECK(a.value() == (1 << fixed_t::FBITS), ">>= 2 failed");
    PASS();
}

//============================================================================
// Edge Case Tests
//============================================================================

void test_limits() {
    TEST("limits");
    CHECK(fixed_t::FMAX == 32767, "FMAX should be 32767");
    CHECK(fixed_t::FMIN == -32768, "FMIN should be -32768");
    CHECK(fixed_t::UNIT == 65536, "UNIT should be 65536");
    CHECK(fixed_t::FMASK == 65535, "FMASK should be 65535");
    PASS();
}

void test_abs() {
    TEST("abs");
    fixed_t a(10), b(-10);
    CHECK(abs(a).value() == (10 << fixed_t::FBITS), "abs(10) failed");
    CHECK(abs(b).value() == (10 << fixed_t::FBITS), "abs(-10) failed");
    PASS();
}

void test_frac() {
    TEST("frac");
    fixed_t a(3.5f);
    fixed_t f = a.frac();
    CHECK(f.value() == (fixed_t::UNIT / 2), "frac(3.5) failed");
    PASS();
}

void test_float_conversion() {
    TEST("float_conversion");
    fixed_t a(2.5f);
    float f = a.Float();
    CHECK(std::abs(f - 2.5f) <= 0.001f, "Float() conversion failed");
    PASS();
}

void test_trunc() {
    TEST("trunc");
    fixed_t a(3.9f);
    CHECK(a.trunc() == 3, "trunc(3.9) should be 3");
    PASS();
}

void test_floor() {
    TEST("floor");
    fixed_t a(3.9f);
    CHECK(a.floor() == 3, "floor(3.9) should be 3");
    PASS();
}

void test_ceil() {
    TEST("ceil");
    fixed_t a(3.1f);
    CHECK(a.ceil() == 4, "ceil(3.1) should be 4");
    PASS();
}

//============================================================================
// Modulus Tests
//============================================================================

void test_modulus() {
    TEST("modulus");
    fixed_t a(7);
    fixed_t b(3);
    fixed_t c = a % b;
    CHECK(c.value() == (1 << fixed_t::FBITS), "7 % 3 should be 1");
    PASS();
}

//============================================================================
// Logical Tests
//============================================================================

void test_logical_not() {
    TEST("logical_not");
    fixed_t a(0);
    fixed_t b(10);
    CHECK(!a == true, "!0 should be true");
    CHECK(!b == false, "!10 should be false");
    PASS();
}

//============================================================================
// Precision Tests
//============================================================================

void test_precision_half() {
    TEST("precision_half");
    fixed_t a(0.5f);
    CHECK(std::abs(a.value() - (fixed_t::UNIT / 2)) <= 1, "0.5 precision failed");
    PASS();
}

void test_precision_quarter() {
    TEST("precision_quarter");
    fixed_t a(0.25f);
    CHECK(std::abs(a.value() - (fixed_t::UNIT / 4)) <= 1, "0.25 precision failed");
    PASS();
}

void test_precision_small() {
    TEST("precision_small");
    fixed_t a(0.001f);
    CHECK(a.value() > 0, "small positive value should be > 0");
    CHECK(a.value() < (fixed_t::UNIT / 10), "0.001 should be less than 0.1");
    PASS();
}

//============================================================================
// Negative Value Tests
//============================================================================

void test_negative_addition() {
    TEST("negative_addition");
    fixed_t a(-5), b(3);
    fixed_t c = a + b;
    CHECK(c.value() == (-2 << fixed_t::FBITS), "-5 + 3 should be -2");
    PASS();
}

void test_negative_multiplication() {
    TEST("negative_multiplication");
    fixed_t a(-4), b(2);
    fixed_t c = a * b;
    CHECK(c.value() == (-8 << fixed_t::FBITS), "-4 * 2 should be -8");
    PASS();
}

void test_negative_division() {
    TEST("negative_division");
    fixed_t a(-20), b(4);
    fixed_t c = a / b;
    CHECK(c.value() == (-5 << fixed_t::FBITS), "-20 / 4 should be -5");
    PASS();
}

//============================================================================
// Main
//============================================================================

int main() {
    cout << "========================================" << endl;
    cout << "fixed_t Unit Tests" << endl;
    cout << "========================================" << endl;

    cout << "\n[Constructor Tests]" << endl;
    test_default_constructor();
    test_int_constructor();
    test_float_constructor();
    test_double_constructor();
    test_copy_constructor();

    cout << "\n[Arithmetic Tests]" << endl;
    test_addition();
    test_subtraction();
    test_multiplication_fixed_fixed();
    test_multiplication_int_fixed();
    test_multiplication_fixed_int();
    test_division_fixed_fixed();
    test_division_fixed_int();
    test_unary_minus();

    cout << "\n[Assignment Operator Tests]" << endl;
    test_assignment();
    test_add_assignment();
    test_sub_assignment();
    test_mul_assignment_fixed();
    test_mul_assignment_int();
    test_div_assignment_fixed();
    test_div_assignment_int();

    cout << "\n[Comparison Operator Tests]" << endl;
    test_equality();
    test_inequality();
    test_less_than();
    test_greater_than();
    test_less_equal();
    test_greater_equal();

    cout << "\n[Bit Shift Tests]" << endl;
    test_left_shift();
    test_right_shift();
    test_left_shift_assignment();
    test_right_shift_assignment();

    cout << "\n[Edge Case Tests]" << endl;
    test_limits();
    test_abs();
    test_frac();
    test_float_conversion();
    test_trunc();
    test_floor();
    test_ceil();

    cout << "\n[Modulus Tests]" << endl;
    test_modulus();

    cout << "\n[Logical Tests]" << endl;
    test_logical_not();

    cout << "\n[Precision Tests]" << endl;
    test_precision_half();
    test_precision_quarter();
    test_precision_small();

    cout << "\n[Negative Value Tests]" << endl;
    test_negative_addition();
    test_negative_multiplication();
    test_negative_division();

    cout << "\n========================================" << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;
    cout << "========================================" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
