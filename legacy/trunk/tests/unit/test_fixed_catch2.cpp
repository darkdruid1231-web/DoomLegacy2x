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
/// \brief Unit tests for fixed_t arithmetic operations using Catch2.

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <cmath>
#include "m_fixed.h"

//============================================================================
// Constructor Tests
//============================================================================

TEST_CASE("fixed_t default constructor", "[fixed_t][constructor]") {
    fixed_t f;
    f = fixed_t(0);
    REQUIRE(f.value() == 0);
}

TEST_CASE("fixed_t int constructor", "[fixed_t][constructor]") {
    fixed_t f(10);
    REQUIRE(f.value() == (10 << fixed_t::FBITS));
}

TEST_CASE("fixed_t float constructor", "[fixed_t][constructor]") {
    fixed_t f(2.5f);
    float expected = 2.5f * float(fixed_t::UNIT);
    REQUIRE(std::abs(f.value() - expected) <= 1);
}

TEST_CASE("fixed_t double constructor", "[fixed_t][constructor]") {
    fixed_t f(3.14159);
    double expected = 3.14159 * double(fixed_t::UNIT);
    REQUIRE(std::abs(f.value() - expected) <= 1);
}

TEST_CASE("fixed_t copy constructor", "[fixed_t][constructor]") {
    fixed_t a(42);
    fixed_t b(a);
    REQUIRE(b.value() == a.value());
}

//============================================================================
// Arithmetic Tests
//============================================================================

TEST_CASE("fixed_t addition", "[fixed_t][arithmetic]") {
    fixed_t a(10), b(20);
    fixed_t c = a + b;
    REQUIRE(c.value() == (30 << fixed_t::FBITS));
}

TEST_CASE("fixed_t subtraction", "[fixed_t][arithmetic]") {
    fixed_t a(20), b(5);
    fixed_t c = a - b;
    REQUIRE(c.value() == (15 << fixed_t::FBITS));
}

TEST_CASE("fixed_t multiplication fixed*fixed", "[fixed_t][arithmetic]") {
    fixed_t a(4), b(2);
    fixed_t c = a * b;
    REQUIRE(c.value() == (8 << fixed_t::FBITS));
}

TEST_CASE("fixed_t multiplication int*fixed", "[fixed_t][arithmetic]") {
    fixed_t a(4);
    fixed_t c = 3 * a;
    REQUIRE(c.value() == (12 << fixed_t::FBITS));
}

TEST_CASE("fixed_t multiplication fixed*int", "[fixed_t][arithmetic]") {
    fixed_t a(4);
    fixed_t c = a * 3;
    REQUIRE(c.value() == (12 << fixed_t::FBITS));
}

TEST_CASE("fixed_t division fixed/fixed", "[fixed_t][arithmetic]") {
    fixed_t a(20), b(4);
    fixed_t c = a / b;
    REQUIRE(c.value() == (5 << fixed_t::FBITS));
}

TEST_CASE("fixed_t division fixed/int", "[fixed_t][arithmetic]") {
    fixed_t a(20);
    fixed_t c = a / 4;
    REQUIRE(c.value() == (5 << fixed_t::FBITS));
}

TEST_CASE("fixed_t unary minus", "[fixed_t][arithmetic]") {
    fixed_t a(10);
    fixed_t b = -a;
    REQUIRE(b.value() == -a.value());
}

//============================================================================
// Assignment Operator Tests
//============================================================================

TEST_CASE("fixed_t assignment", "[fixed_t][assignment]") {
    fixed_t a(10), b;
    b = a;
    REQUIRE(b.value() == a.value());
}

TEST_CASE("fixed_t add assignment", "[fixed_t][assignment]") {
    fixed_t a(10);
    a += fixed_t(5);
    REQUIRE(a.value() == (15 << fixed_t::FBITS));
}

TEST_CASE("fixed_t sub assignment", "[fixed_t][assignment]") {
    fixed_t a(10);
    a -= fixed_t(3);
    REQUIRE(a.value() == (7 << fixed_t::FBITS));
}

TEST_CASE("fixed_t mul assignment fixed", "[fixed_t][assignment]") {
    fixed_t a(10);
    a *= fixed_t(2);
    REQUIRE(a.value() == (20 << fixed_t::FBITS));
}

TEST_CASE("fixed_t mul assignment int", "[fixed_t][assignment]") {
    fixed_t a(10);
    a *= 2;
    REQUIRE(a.value() == (20 << fixed_t::FBITS));
}

TEST_CASE("fixed_t div assignment fixed", "[fixed_t][assignment]") {
    fixed_t a(20);
    a /= fixed_t(4);
    REQUIRE(a.value() == (5 << fixed_t::FBITS));
}

TEST_CASE("fixed_t div assignment int", "[fixed_t][assignment]") {
    fixed_t a(20);
    a /= 4;
    REQUIRE(a.value() == (5 << fixed_t::FBITS));
}

//============================================================================
// Comparison Operator Tests
//============================================================================

TEST_CASE("fixed_t equality", "[fixed_t][comparison]") {
    fixed_t a(10), b(10), c(20);
    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
}

TEST_CASE("fixed_t inequality", "[fixed_t][comparison]") {
    fixed_t a(10), b(20);
    REQUIRE(a != b);
    REQUIRE_FALSE(a != a);
}

TEST_CASE("fixed_t less than", "[fixed_t][comparison]") {
    fixed_t a(10), b(20);
    REQUIRE(a < b);
    REQUIRE_FALSE(b < a);
}

TEST_CASE("fixed_t greater than", "[fixed_t][comparison]") {
    fixed_t a(20), b(10);
    REQUIRE(a > b);
    REQUIRE_FALSE(b > a);
}

TEST_CASE("fixed_t less or equal", "[fixed_t][comparison]") {
    fixed_t a(10), b(10), c(20);
    REQUIRE(a <= b);
    REQUIRE(a <= c);
    REQUIRE_FALSE(c <= a);
}

TEST_CASE("fixed_t greater or equal", "[fixed_t][comparison]") {
    fixed_t a(20), b(20), c(10);
    REQUIRE(a >= b);
    REQUIRE(a >= c);
    REQUIRE_FALSE(c >= a);
}

//============================================================================
// Bit Shift Tests
//============================================================================

TEST_CASE("fixed_t left shift", "[fixed_t][bitwise]") {
    fixed_t a(1);
    fixed_t b = a << 1;
    REQUIRE(b.value() == (2 << fixed_t::FBITS));
}

TEST_CASE("fixed_t right shift", "[fixed_t][bitwise]") {
    fixed_t a(2);
    fixed_t b = a >> 1;
    REQUIRE(b.value() == (1 << fixed_t::FBITS));
}

TEST_CASE("fixed_t left shift assignment", "[fixed_t][bitwise]") {
    fixed_t a(1);
    a <<= 2;
    REQUIRE(a.value() == (4 << fixed_t::FBITS));
}

TEST_CASE("fixed_t right shift assignment", "[fixed_t][bitwise]") {
    fixed_t a(4);
    a >>= 2;
    REQUIRE(a.value() == (1 << fixed_t::FBITS));
}

//============================================================================
// Edge Case Tests
//============================================================================

TEST_CASE("fixed_t limits", "[fixed_t][constants]") {
    REQUIRE(fixed_t::FMAX == 32767);
    REQUIRE(fixed_t::FMIN == -32768);
    REQUIRE(fixed_t::UNIT == 65536);
    REQUIRE(fixed_t::FMASK == 65535);
}

TEST_CASE("fixed_t abs", "[fixed_t][utility]") {
    fixed_t a(10), b(-10);
    REQUIRE(abs(a).value() == (10 << fixed_t::FBITS));
    REQUIRE(abs(b).value() == (10 << fixed_t::FBITS));
}

TEST_CASE("fixed_t frac", "[fixed_t][utility]") {
    fixed_t a(3.5f);
    fixed_t f = a.frac();
    REQUIRE(f.value() == (fixed_t::UNIT / 2));
}

TEST_CASE("fixed_t float conversion", "[fixed_t][conversion]") {
    fixed_t a(2.5f);
    float f = a.Float();
    REQUIRE(std::abs(f - 2.5f) <= 0.001f);
}

TEST_CASE("fixed_t trunc", "[fixed_t][utility]") {
    fixed_t a(3.9f);
    REQUIRE(a.trunc() == 3);
}

TEST_CASE("fixed_t floor", "[fixed_t][utility]") {
    fixed_t a(3.9f);
    REQUIRE(a.floor() == 3);
}

TEST_CASE("fixed_t ceil", "[fixed_t][utility]") {
    fixed_t a(3.1f);
    REQUIRE(a.ceil() == 4);
}

//============================================================================
// Modulus Tests
//============================================================================

TEST_CASE("fixed_t modulus", "[fixed_t][arithmetic]") {
    fixed_t a(7);
    fixed_t b(3);
    fixed_t c = a % b;
    REQUIRE(c.value() == (1 << fixed_t::FBITS));
}

//============================================================================
// Logical Tests
//============================================================================

TEST_CASE("fixed_t logical not", "[fixed_t][logical]") {
    fixed_t a(0);
    fixed_t b(10);
    REQUIRE(!a == true);
    REQUIRE(!b == false);
}

//============================================================================
// Precision Tests
//============================================================================

TEST_CASE("fixed_t precision half", "[fixed_t][precision]") {
    fixed_t a(0.5f);
    REQUIRE(std::abs(a.value() - (fixed_t::UNIT / 2)) <= 1);
}

TEST_CASE("fixed_t precision quarter", "[fixed_t][precision]") {
    fixed_t a(0.25f);
    REQUIRE(std::abs(a.value() - (fixed_t::UNIT / 4)) <= 1);
}

TEST_CASE("fixed_t precision small", "[fixed_t][precision]") {
    fixed_t a(0.001f);
    REQUIRE(a.value() > 0);
    REQUIRE(a.value() < (fixed_t::UNIT / 10));
}

//============================================================================
// Negative Value Tests
//============================================================================

TEST_CASE("fixed_t negative addition", "[fixed_t][arithmetic][negative]") {
    fixed_t a(-5), b(3);
    fixed_t c = a + b;
    REQUIRE(c.value() == (-2 << fixed_t::FBITS));
}

TEST_CASE("fixed_t negative multiplication", "[fixed_t][arithmetic][negative]") {
    fixed_t a(-4), b(2);
    fixed_t c = a * b;
    REQUIRE(c.value() == (-8 << fixed_t::FBITS));
}

TEST_CASE("fixed_t negative division", "[fixed_t][arithmetic][negative]") {
    fixed_t a(-20), b(4);
    fixed_t c = a / b;
    REQUIRE(c.value() == (-5 << fixed_t::FBITS));
}
