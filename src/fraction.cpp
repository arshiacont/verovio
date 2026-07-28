/////////////////////////////////////////////////////////////////////////////
// Name:        fraction.cpp
// Author:      Laurent Pugin
// Created:     2024
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "fraction.h"

//----------------------------------------------------------------------------

#include <cassert>
#include <cstdint>
#include <limits>
#include <math.h>
#include <numeric>
#include <stdexcept>

//----------------------------------------------------------------------------

namespace vrv {

namespace {

int NarrowFractionComponent(std::int64_t value)
{
    if ((value < std::numeric_limits<int>::min()) || (value > std::numeric_limits<int>::max())) {
        throw std::overflow_error("Fraction value is outside the supported integer range");
    }
    return static_cast<int>(value);
}

Fraction MakeFraction(std::int64_t numerator, std::int64_t denominator)
{
    if (denominator < 0) {
        numerator = -numerator;
        denominator = -denominator;
    }

    const std::int64_t gcd = std::gcd(numerator, denominator);
    if (gcd != 0) {
        numerator /= gcd;
        denominator /= gcd;
    }

    return Fraction(NarrowFractionComponent(numerator), NarrowFractionComponent(denominator));
}

} // namespace

//----------------------------------------------------------------------------
// Fraction
//----------------------------------------------------------------------------

Fraction::Fraction(int num, int denom)
{
    m_numerator = num;
    if (denom == 0) {
        LogDebug("Denominator cannot be zero.");
        denom = 1;
    }
    m_denominator = denom;
    this->Reduce();
}

Fraction::Fraction(data_DURATION duration)
{
    duration = vrv::DurationMin(duration, DURATION_2048);
    duration = vrv::DurationMax(duration, DURATION_maxima);
    int den = pow(2, (duration + 1));
    m_numerator = 8;
    m_denominator = den;
    this->Reduce();
}

Fraction Fraction::operator+(const Fraction &other) const
{
    const std::int64_t gcd = std::gcd(m_denominator, other.m_denominator);
    const std::int64_t leftMultiplier = other.m_denominator / gcd;
    const std::int64_t rightMultiplier = m_denominator / gcd;
    const std::int64_t numerator
        = static_cast<std::int64_t>(m_numerator) * leftMultiplier + other.m_numerator * rightMultiplier;
    const std::int64_t denominator = static_cast<std::int64_t>(m_denominator) * leftMultiplier;
    return MakeFraction(numerator, denominator);
}

Fraction Fraction::operator-(const Fraction &other) const
{
    const std::int64_t gcd = std::gcd(m_denominator, other.m_denominator);
    const std::int64_t leftMultiplier = other.m_denominator / gcd;
    const std::int64_t rightMultiplier = m_denominator / gcd;
    const std::int64_t numerator
        = static_cast<std::int64_t>(m_numerator) * leftMultiplier - other.m_numerator * rightMultiplier;
    const std::int64_t denominator = static_cast<std::int64_t>(m_denominator) * leftMultiplier;
    return MakeFraction(numerator, denominator);
}

Fraction Fraction::operator*(const Fraction &other) const
{
    const std::int64_t leftReduction = std::gcd<std::int64_t>(m_numerator, other.m_denominator);
    const std::int64_t rightReduction = std::gcd<std::int64_t>(other.m_numerator, m_denominator);
    const std::int64_t numerator
        = (m_numerator / leftReduction) * static_cast<std::int64_t>(other.m_numerator / rightReduction);
    const std::int64_t denominator
        = (m_denominator / rightReduction) * static_cast<std::int64_t>(other.m_denominator / leftReduction);
    return MakeFraction(numerator, denominator);
}

Fraction Fraction::operator/(const Fraction &other) const
{
    if (other.m_numerator == 0) {
        LogDebug("Cannot divide by zero.");
        return *this;
    }
    const std::int64_t numeratorReduction = std::gcd<std::int64_t>(m_numerator, other.m_numerator);
    const std::int64_t denominatorReduction = std::gcd<std::int64_t>(other.m_denominator, m_denominator);
    const std::int64_t numerator
        = (m_numerator / numeratorReduction)
        * static_cast<std::int64_t>(other.m_denominator / denominatorReduction);
    const std::int64_t denominator
        = (m_denominator / denominatorReduction)
        * static_cast<std::int64_t>(other.m_numerator / numeratorReduction);
    return MakeFraction(numerator, denominator);
}

Fraction Fraction::operator%(const Fraction &other) const
{
    if (other.m_numerator == 0) {
        LogDebug("Cannot divide by zero.");
        return *this;
    }

    const std::int64_t gcd = std::gcd(m_denominator, other.m_denominator);
    const std::int64_t leftMultiplier = other.m_denominator / gcd;
    const std::int64_t rightMultiplier = m_denominator / gcd;
    const std::int64_t leftNumerator = static_cast<std::int64_t>(m_numerator) * leftMultiplier;
    const std::int64_t rightNumerator = static_cast<std::int64_t>(other.m_numerator) * rightMultiplier;
    const std::int64_t commonDenominator = static_cast<std::int64_t>(m_denominator) * leftMultiplier;
    return MakeFraction(leftNumerator % rightNumerator, commonDenominator);
}

bool Fraction::operator==(const Fraction &other) const
{
    return static_cast<std::int64_t>(m_numerator) * other.m_denominator
        == static_cast<std::int64_t>(other.m_numerator) * m_denominator;
}

std::strong_ordering Fraction::operator<=>(const Fraction &other) const
{
    return static_cast<std::int64_t>(m_numerator) * other.m_denominator
        <=> static_cast<std::int64_t>(other.m_numerator) * m_denominator;
}

double Fraction::ToDouble() const
{
    return static_cast<double>(m_numerator) / m_denominator;
}

std::string Fraction::ToString() const
{
    return StringFormat("%d/%d", m_numerator, m_denominator);
}

void Fraction::Reduce()
{
    if (m_denominator < 0) { // Keep the denominator positive
        m_numerator = -m_numerator;
        m_denominator = -m_denominator;
    }
    const int gcdVal = std::gcd(m_numerator, m_denominator);
    if (gcdVal != 1) {
        m_numerator /= gcdVal;
        m_denominator /= gcdVal;
    }
}

std::pair<data_DURATION, Fraction> Fraction::ToDur() const
{
    if (m_numerator == 0) return { DURATION_NONE, 0 };

    int value = ceil(log2((double)m_denominator / (double)m_numerator * 8)) - 1;
    data_DURATION dur = static_cast<data_DURATION>(value);
    dur = vrv::DurationMax(DURATION_maxima, dur);
    dur = vrv::DurationMin(DURATION_2048, dur);

    Fraction remainder = *this - Fraction(dur);
    // Making sure we would not be triggering an infinite loop when looping over the remainder
    if ((remainder >= *this) || (remainder < 0)) remainder = 0;
    return { dur, remainder };
}

void Fraction::Reduce(int &numerator, int &denominator)
{
    Fraction fraction(numerator, denominator);
    numerator = fraction.GetNumerator();
    denominator = fraction.GetDenominator();
}

} // namespace vrv
