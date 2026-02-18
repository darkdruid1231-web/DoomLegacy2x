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
/// \brief Unit tests for vec_t (vector math) operations.

#include <iostream>
#include <cmath>
#include <string>
#include "vect.h"

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
} while(0)

#define ASSERT_EQ(expected, actual, msg) do { \
    if ((expected) != (actual)) { \
        FAIL(msg); \
        cout << "    Expected: " << (expected) << ", Got: " << (actual) << endl; \
        return; \
    } \
} while(0)

#define ASSERT_NEAR(expected, actual, tolerance, msg) do { \
    if (fabs((expected) - (actual)) > (tolerance)) { \
        FAIL(msg); \
        cout << "    Expected: " << (expected) << " +/- " << (tolerance) << ", Got: " << (actual) << endl; \
        return; \
    } \
} while(0)


// Test default constructor
void test_default_constructor()
{
    TEST("vec_t default constructor creates zero vector");
    vec_t<float> v;
    ASSERT_EQ(0.0f, v.x, "x component is zero");
    ASSERT_EQ(0.0f, v.y, "y component is zero");
    ASSERT_EQ(0.0f, v.z, "z component is zero");
    PASS();
}

// Test parameterized constructor
void test_parameterized_constructor()
{
    TEST("vec_t parameterized constructor sets components");
    vec_t<float> v(1.0f, 2.0f, 3.0f);
    ASSERT_EQ(1.0f, v.x, "x component set correctly");
    ASSERT_EQ(2.0f, v.y, "y component set correctly");
    ASSERT_EQ(3.0f, v.z, "z component set correctly");
    PASS();
}

// Test copy constructor
void test_copy_constructor()
{
    TEST("vec_t copy constructor works");
    vec_t<float> v1(1.5f, 2.5f, 3.5f);
    vec_t<float> v2(v1);
    ASSERT_EQ(v1.x, v2.x, "x component copied correctly");
    ASSERT_EQ(v1.y, v2.y, "y component copied correctly");
    ASSERT_EQ(v1.z, v2.z, "z component copied correctly");
    PASS();
}

// Test assignment operator
void test_assignment_operator()
{
    TEST("vec_t assignment operator works");
    vec_t<float> v1(1.0f, 2.0f, 3.0f);
    vec_t<float> v2;
    v2 = v1;
    ASSERT_EQ(v1.x, v2.x, "x component assigned correctly");
    ASSERT_EQ(v1.y, v2.y, "y component assigned correctly");
    ASSERT_EQ(v1.z, v2.z, "z component assigned correctly");
    PASS();
}

// Test Set method
void test_set_method()
{
    TEST("vec_t Set method works");
    vec_t<float> v;
    v.Set(5.0f, 10.0f, 15.0f);
    ASSERT_EQ(5.0f, v.x, "x component set correctly");
    ASSERT_EQ(10.0f, v.y, "y component set correctly");
    ASSERT_EQ(15.0f, v.z, "z component set correctly");
    PASS();
}

// Test addition operator
void test_addition_operator()
{
    TEST("vec_t addition operator works");
    vec_t<float> v1(1.0f, 2.0f, 3.0f);
    vec_t<float> v2(4.0f, 5.0f, 6.0f);
    vec_t<float> v3 = v1 + v2;
    ASSERT_EQ(5.0f, v3.x, "x component adds correctly");
    ASSERT_EQ(7.0f, v3.y, "y component adds correctly");
    ASSERT_EQ(9.0f, v3.z, "z component adds correctly");
    PASS();
}

// Test subtraction operator
void test_subtraction_operator()
{
    TEST("vec_t subtraction operator works");
    vec_t<float> v1(5.0f, 7.0f, 9.0f);
    vec_t<float> v2(1.0f, 2.0f, 3.0f);
    vec_t<float> v3 = v1 - v2;
    ASSERT_EQ(4.0f, v3.x, "x component subtracts correctly");
    ASSERT_EQ(5.0f, v3.y, "y component subtracts correctly");
    ASSERT_EQ(6.0f, v3.z, "z component subtracts correctly");
    PASS();
}

// Test scalar multiplication
void test_scalar_multiplication()
{
    TEST("vec_t scalar multiplication works");
    vec_t<float> v1(1.0f, 2.0f, 3.0f);
    vec_t<float> v2 = v1 * 3.0f;
    ASSERT_EQ(3.0f, v2.x, "x component multiplies correctly");
    ASSERT_EQ(6.0f, v2.y, "y component multiplies correctly");
    ASSERT_EQ(9.0f, v2.z, "z component multiplies correctly");
    PASS();
}

// Test scalar division
void test_scalar_division()
{
    TEST("vec_t scalar division works");
    vec_t<float> v1(6.0f, 9.0f, 12.0f);
    vec_t<float> v2 = v1 / 3.0f;
    ASSERT_EQ(2.0f, v2.x, "x component divides correctly");
    ASSERT_EQ(3.0f, v2.y, "y component divides correctly");
    ASSERT_EQ(4.0f, v2.z, "z component divides correctly");
    PASS();
}

// Test compound addition
void test_compound_addition()
{
    TEST("vec_t compound addition (+=) works");
    vec_t<float> v(1.0f, 2.0f, 3.0f);
    vec_t<float> v2(4.0f, 5.0f, 6.0f);
    v += v2;
    ASSERT_EQ(5.0f, v.x, "x component adds correctly");
    ASSERT_EQ(7.0f, v.y, "y component adds correctly");
    ASSERT_EQ(9.0f, v.z, "z component adds correctly");
    PASS();
}

// Test compound subtraction
void test_compound_subtraction()
{
    TEST("vec_t compound subtraction (-=) works");
    vec_t<float> v(10.0f, 8.0f, 6.0f);
    vec_t<float> v2(3.0f, 2.0f, 1.0f);
    v -= v2;
    ASSERT_EQ(7.0f, v.x, "x component subtracts correctly");
    ASSERT_EQ(6.0f, v.y, "y component subtracts correctly");
    ASSERT_EQ(5.0f, v.z, "z component subtracts correctly");
    PASS();
}

// Test compound scalar multiplication
void test_compound_scalar_multiplication()
{
    TEST("vec_t compound scalar multiplication (*=) works");
    vec_t<float> v(1.0f, 2.0f, 3.0f);
    v *= 2.0f;
    ASSERT_EQ(2.0f, v.x, "x component multiplies correctly");
    ASSERT_EQ(4.0f, v.y, "y component multiplies correctly");
    ASSERT_EQ(6.0f, v.z, "z component multiplies correctly");
    PASS();
}

// Test compound scalar division
void test_compound_scalar_division()
{
    TEST("vec_t compound scalar division (/=) works");
    vec_t<float> v(10.0f, 20.0f, 30.0f);
    v /= 10.0f;
    ASSERT_EQ(1.0f, v.x, "x component divides correctly");
    ASSERT_EQ(2.0f, v.y, "y component divides correctly");
    ASSERT_EQ(3.0f, v.z, "z component divides correctly");
    PASS();
}

// Test unary minus
void test_unary_minus()
{
    TEST("vec_t unary minus works");
    vec_t<float> v1(1.0f, -2.0f, 3.0f);
    vec_t<float> v2 = -v1;
    ASSERT_EQ(-1.0f, v2.x, "x component negated correctly");
    ASSERT_EQ(2.0f, v2.y, "y component negated correctly");
    ASSERT_EQ(-3.0f, v2.z, "z component negated correctly");
    PASS();
}

// Test dot product
void test_dot_product()
{
    TEST("vec_t dot product works");
    vec_t<float> v1(1.0f, 0.0f, 0.0f);
    vec_t<float> v2(0.0f, 1.0f, 0.0f);
    float d = dot(v1, v2);
    ASSERT_EQ(0.0f, d, "perpendicular vectors have zero dot product");

    vec_t<float> v3(1.0f, 2.0f, 3.0f);
    vec_t<float> v4(4.0f, 5.0f, 6.0f);
    d = dot(v3, v4);
    ASSERT_EQ(32.0f, d, "dot product calculation correct");
    PASS();
}

// Test cross product
void test_cross_product()
{
    TEST("vec_t cross product works");
    // Cross product of x-axis and y-axis should give z-axis
    vec_t<float> v1(1.0f, 0.0f, 0.0f);
    vec_t<float> v2(0.0f, 1.0f, 0.0f);
    vec_t<float> v3 = cross(v1, v2);
    ASSERT_NEAR(0.0f, v3.x, 0.001f, "x component of cross product");
    ASSERT_NEAR(0.0f, v3.y, 0.001f, "y component of cross product");
    ASSERT_NEAR(1.0f, v3.z, 0.001f, "z component of cross product");
    PASS();
}

// Test Norm (magnitude)
void test_norm()
{
    TEST("vec_t Norm calculates magnitude correctly");
    vec_t<float> v(3.0f, 4.0f, 0.0f);
    float n = v.Norm();
    ASSERT_NEAR(5.0f, n, 0.001f, "3-4-0 triangle has magnitude 5");
    PASS();
}

// Test Norm2 (squared magnitude)
void test_norm2()
{
    TEST("vec_t Norm2 calculates squared magnitude correctly");
    vec_t<float> v(3.0f, 4.0f, 0.0f);
    float n2 = v.Norm2();
    ASSERT_EQ(25.0f, n2, "3-4-0 triangle has squared magnitude 25");
    PASS();
}

// Test XYNorm2 (squared XY magnitude)
void test_xy_norm2()
{
    TEST("vec_t XYNorm2 calculates squared XY magnitude correctly");
    vec_t<float> v(3.0f, 4.0f, 5.0f);
    float n2 = v.XYNorm2();
    ASSERT_EQ(25.0f, n2, "3-4 XY components have squared magnitude 25");
    PASS();
}

// Test equality operator
void test_equality()
{
    TEST("vec_t equality operator works");
    vec_t<float> v1(1.0f, 2.0f, 3.0f);
    vec_t<float> v2(1.0f, 2.0f, 3.0f);
    ASSERT_EQ(1, v1 == v2, "identical vectors are equal");

    vec_t<float> v3(1.0f, 2.0f, 4.0f);
    ASSERT_EQ(0, v1 == v3, "different vectors are not equal");
    PASS();
}

// Test inequality operator
void test_inequality()
{
    TEST("vec_t inequality operator works");
    vec_t<float> v1(1.0f, 2.0f, 3.0f);
    vec_t<float> v2(1.0f, 2.0f, 3.0f);
    ASSERT_EQ(0, v1 != v2, "identical vectors are not unequal");

    vec_t<float> v3(1.0f, 2.0f, 4.0f);
    ASSERT_EQ(1, v1 != v3, "different vectors are unequal");
    PASS();
}

// Test bit shift operators
void test_bit_shift()
{
    TEST("vec_t left shift (<<) works");
    vec_t<int> v(1, 2, 4);
    vec_t<int> v2 = v << 1;
    ASSERT_EQ(2, v2.x, "x component shifted correctly");
    ASSERT_EQ(4, v2.y, "y component shifted correctly");
    ASSERT_EQ(8, v2.z, "z component shifted correctly");
    PASS();

    TEST("vec_t right shift (>>) works");
    v2 = v >> 1;
    ASSERT_EQ(0, v2.x, "x component shifted correctly");
    ASSERT_EQ(1, v2.y, "y component shifted correctly");
    ASSERT_EQ(2, v2.z, "z component shifted correctly");
    PASS();
}

// Test polar coordinate methods
void test_polar_coordinates()
{
    TEST("vec_t polar coordinate methods exist");
    vec_t<float> v(1.0f, 0.0f, 0.0f);
    float phi = v.Phi();
    ASSERT_NEAR(0.0f, phi, 0.001f, "Phi on x-axis is 0");

    // Test theta
    vec_t<float> v2(0.0f, 0.0f, 1.0f);
    float theta = v2.Theta();
    ASSERT_NEAR(0.0f, theta, 0.001f, "Theta on z-axis is 0");
    PASS();
}


int main()
{
    cout << "=== Vector Math Tests ===" << endl;

    // Constructor tests
    test_default_constructor();
    test_parameterized_constructor();
    test_copy_constructor();
    test_assignment_operator();
    test_set_method();

    // Arithmetic tests
    test_addition_operator();
    test_subtraction_operator();
    test_scalar_multiplication();
    test_scalar_division();
    test_compound_addition();
    test_compound_subtraction();
    test_compound_scalar_multiplication();
    test_compound_scalar_division();
    test_unary_minus();

    // Advanced operations
    test_dot_product();
    test_cross_product();
    test_norm();
    test_norm2();
    test_xy_norm2();
    test_polar_coordinates();

    // Comparison tests
    test_equality();
    test_inequality();
    test_bit_shift();

    cout << endl;
    cout << "Results: " << tests_passed << "/" << tests_run << " tests passed" << endl;

    return (tests_passed == tests_run) ? 0 : 1;
}
