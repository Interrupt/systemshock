/*

Copyright (C) 2015-2018 Night Dive Studios, LLC.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/*
 * $Header: r:/prj/lib/src/fixpp/RCS/fixpp.h 1.45 1994/08/08 18:27:45 ept Exp $
 */

/*
I'd like to dedicate this fixpoint library to Dan and Matt for their
inspirational bravery, and to C++ for being such a complex and neurotic
language.
*/

#ifndef __FIXPP_H
#define __FIXPP_H

#include <istream>
#include <cstdio>
#include <cstdlib>

extern "C" {
//#include "mprintf.h"
#include "fix.h" // A big thank you to Dan and Matt.
}

// How many bits to shift an integer up to make it a fixpoint.
// ===========================================================
#ifdef FIXPOINT_SHIFTUP
#define SHIFTUP FIXPOINT_SHIFTUP
#else
#define SHIFTUP 16 // 16:16 default format.
#endif

#define SHIFTMULTIPLIER (1 << SHIFTUP)

// Here are some flags for your convenience.
// =========================================

// Define this for some misc. debugging things like touch()
// ========================================================
//#define FIXDEBUG

#ifdef FIXDEBUG
#define CLICK(c) c += (Fixpoint::click_bool)
#else
#define CLICK(c)
#endif

// Here is a nice forward declaration.
// ===================================

class Fixpoint;

#define Q Fixpoint

class Fixpoint {

    friend Fixpoint rawConstruct(int32_t l);

  public:
    // The data is stored here.
    // ========================
    int32_t val;

    // Some invasive functions to get right at the internal rep.
    // What?  Me not secure?  I'm no fascist.
    // =========================================================

    uint32_t bits();
    void setbits(uint32_t ul);

    // Constructors.
    // =============

    Fixpoint();

    Fixpoint(const Fixpoint &);

    Fixpoint(int32_t i);

    Fixpoint(uint32_t i);

    Fixpoint(double);

    // Conversions.
    // ============

    double to_double() const;

    float to_float() const;

    int32_t to_int() const;

    fix to_fix() const;

    fixang to_fixang() const;

    // Reverse Conversions.
    // ====================

    void fix_to(fix);

    void fixang_to(fixang);

    // Assignments.
    // ============

    // REMOVED!!!!

    // Arithmetic operators (homogeneous)!!
    // ====================================

    Fixpoint &operator+=(Fixpoint);

    Fixpoint &operator-=(Fixpoint);

    Fixpoint &operator*=(Fixpoint);

    Fixpoint &operator/=(Fixpoint);

    Fixpoint &operator<<=(uint32_t n);

    Fixpoint &operator>>=(uint32_t n);

    Fixpoint operator-() const;

    Fixpoint operator+() const;

    int32_t operator<(const Fixpoint &) const;

    int32_t operator>(const Fixpoint &) const;

    int32_t operator<=(const Fixpoint &fp2) const;

    int32_t operator>=(const Fixpoint &fp2) const;

    int32_t operator==(const Fixpoint &fp2) const;

    int32_t operator!=(const Fixpoint &fp2) const;

    // Signed shifts
    // =============

    void shift(int32_t n);
    Fixpoint shifted(int32_t n) const;

    // Fast comparisons with zero (maybe... perhaps Q(0) isn't so slow after all)
    // (and a trip down memory lane for FORTRAN-ites)
    // ====================================

    int32_t gt_zero() const;

    int32_t ge_zero() const;

    int32_t eq_zero() const;

    int32_t ne_zero() const;

    int32_t le_zero() const;

    int32_t lt_zero() const;

    // Friendly math function declarations.
    // ====================================

    friend inline Fixpoint sqrt(Fixpoint);
    friend inline Fixpoint exp(Fixpoint);
    friend inline int32_t floor(Fixpoint);
    friend inline Fixpoint sin(Fixpoint);
    friend inline Fixpoint cos(Fixpoint);
    friend inline Fixpoint tan(Fixpoint);
    friend inline Fixpoint acos(Fixpoint);
    friend inline Fixpoint asin(Fixpoint);
    friend inline void sincos(Fixpoint ang, Fixpoint *sn, Fixpoint *cs);
    friend inline Fixpoint atan2(Fixpoint, Fixpoint);
    friend inline Fixpoint fsin(Fixpoint);
    friend inline Fixpoint fcos(Fixpoint);
    friend inline void fsincos(Fixpoint ang, Fixpoint *sn, Fixpoint *cs);
    friend inline Fixpoint abs(Fixpoint);

#ifdef FIXDEBUG

    friend char *bitdump(Fixpoint &);

    // Reporting.
    // ==========

    static uint8_t click_bool;

    static uint32_t constructor_void, constructor_Fixpoint, constructor_int, constructor_uint, constructor_lint,
        constructor_ulint, constructor_double;

    static uint32_t ass_Fixpoint, ass_int, ass_uint, ass_lint, ass_ulint, ass_double;

    static uint32_t binary_add, binary_sub, binary_mul, binary_div;

    static uint32_t add_eq, sub_eq, mul_eq, div_eq;

    static uint32_t unary_minus, unary_plus;

    static uint32_t cond_l, cond_g, cond_le, cond_ge, cond_eq, cond_neq;

    static void report_on(void) { click_bool = 1; }
    static void report_off(void) { click_bool = 0; }

    static void report(std::ostream &);
    static void report(void);
    static void reset_report(void);

#endif /* FIXDEBUG */

} /* Blessed be!! */;

// Constructors
// ============

inline uint32_t Fixpoint::bits() { return (uint32_t)val; }
inline void Fixpoint::setbits(uint32_t ul) { val = ul; }

inline Fixpoint::Fixpoint() { CLICK(constructor_void); } // Hey, why not define our own....

inline Fixpoint::Fixpoint(const Fixpoint &fp) {
    CLICK(constructor_Fixpoint);
    val = fp.val;
}

inline Fixpoint::Fixpoint(int32_t i) {
    CLICK(constructor_int);
    val = i << SHIFTUP;
}

inline Fixpoint::Fixpoint(uint32_t i) {
    CLICK(constructor_uint);
    val = i << SHIFTUP;
}

inline Fixpoint::Fixpoint(double d) {
    CLICK(constructor_double);
    val = (int32_t)(d * SHIFTMULTIPLIER);
}

inline Fixpoint rawConstruct(int32_t l) {
    Fixpoint f;
    f.val = l;
    return f;
}
#define f2Fixpoint(x) (rawConstruct((int32_t)((x)*SHIFTMULTIPLIER)))

// ======================================
//
// Math functions.
//
// ======================================

////////////
//        //
//   +=   //
//        //
////////////
inline Fixpoint &Fixpoint::operator+=(Fixpoint fp2) {
    CLICK(add_eq);

    val += fp2.val;

    return *this;
}

////////////
//        //
//   -=   //
//        //
////////////
inline Fixpoint &Fixpoint::operator-=(Fixpoint fp2) {
    CLICK(sub_eq);

    val -= fp2.val;
    return *this;
}

////////////
//        //
//   *=   //
//        //
////////////
inline Fixpoint &Fixpoint::operator*=(Fixpoint fp2) {
    CLICK(mul_eq);

    val = (int32_t)fix_mul((fix)val, (fix)fp2.val);
    return *this;
}

////////////
//        //
//   /=   //
//        //
////////////
inline Fixpoint &Fixpoint::operator/=(Fixpoint fp2) {
    CLICK(div_eq);

    val = (int32_t)fix_div((fix)val, (fix)fp2.val);
    // val = _fix_do_div(val, fp2.val);
    return *this;
}

inline Fixpoint &Fixpoint::operator<<=(uint32_t n) {
    val <<= n;
    return *this;
}

inline Fixpoint &Fixpoint::operator>>=(uint32_t n) {
    val >>= n;
    return *this;
}

inline Fixpoint operator+(Fixpoint a, Fixpoint b) {
    CLICK(Fixpoint::binary_add);
    a.val += b.val;
    return a;
}

inline Fixpoint operator-(Fixpoint a, Fixpoint b) {
    CLICK(Fixpoint::binary_sub);
    a.val -= b.val;
    return a;
}

inline Fixpoint operator*(Fixpoint a, Fixpoint b) {
    CLICK(Fixpoint::binary_mul);
    a.val = (int32_t)fix_mul((fix)a.val, (fix)b.val);
    return a;
}

inline Fixpoint operator/(Fixpoint a, Fixpoint b) {
    CLICK(Fixpoint::binary_div);
    a.val = (int32_t)fix_div((fix)a.val, (fix)b.val);
    // a.val=_fix_do_div(a.val,b.val);
    return a;
}

///////////
//       //
//   -   //
//       //
///////////
inline Fixpoint Fixpoint::operator-() const {
    Fixpoint ans;

    CLICK(unary_minus);

    ans.val = -this->val;

    return ans;
}

///////////
//       //
//   +   //
//       //
///////////
inline Fixpoint Fixpoint::operator+() const {
    CLICK(unary_plus);

    return *this;
}

inline void Fixpoint::shift(int32_t n) {
    if (n > 0)
        val <<= n;
    else if (n < 0)
        val >>= (-n);
}

inline Fixpoint Fixpoint::shifted(int32_t n) const {
    Fixpoint r(*this);
    if (n > 0)
        r.val <<= n;
    else if (n < 0)
        r.val >>= (-n);
    return r;
}

inline Fixpoint operator<<(Fixpoint p, unsigned int n) {
    p.val <<= n;
    return p;
}

inline Fixpoint operator>>(Fixpoint p, unsigned int n) {
    p.val >>= n;
    return p;
}

// Conversions.
// ============

inline double Fixpoint::to_double() const { return ((double)val) / SHIFTMULTIPLIER; }

inline float Fixpoint::to_float() const { return ((float)val) / SHIFTMULTIPLIER; }

inline int32_t Fixpoint::to_int() const { return (int32_t)(val >> SHIFTUP); }

inline fix Fixpoint::to_fix() const { return (fix)val; }

inline fixang Fixpoint::to_fixang() const {
    Fixpoint temp = *this * f2Fixpoint(0.159154943);

    // for temp, 360 degrees = 1.0.
    // The lower 16 bits of the internal rep is the fixang.

    return (uint16_t)temp.val;
}

inline void Fixpoint::fix_to(fix f) { val = f; }

inline void Fixpoint::fixang_to(fixang f) {
    val = ((int32_t)(int16_t)(f - 1)) + 1;
    *this *= f2Fixpoint(6.283185306);
}

// Comparisons.
// ============

///////////
//       //
//   <   //
//       //
///////////
inline int32_t Fixpoint::operator<(const Fixpoint &fp2) const {
    CLICK(cond_l);

    return this->val < fp2.val;
}

///////////
//       //
//   >   //
//       //
///////////
inline int32_t Fixpoint::operator>(const Fixpoint &fp2) const {
    CLICK(cond_g);

    return this->val > fp2.val;
}

////////////
//        //
//   <=   //
//        //
////////////
inline int32_t Fixpoint::operator<=(const Fixpoint &fp2) const {
    CLICK(cond_le);

    return this->val <= fp2.val;
}

////////////
//        //
//   >=   //
//        //
////////////
inline int32_t Fixpoint::operator>=(const Fixpoint &fp2) const {
    CLICK(cond_ge);

    return this->val >= fp2.val;
}

////////////
//        //
//   ==   //
//        //
////////////
inline int32_t Fixpoint::operator==(const Fixpoint &fp2) const {
    CLICK(cond_eq);

    return this->val == fp2.val;
}

////////////
//        //
//   !=   //
//        //
////////////
inline int32_t Fixpoint::operator!=(const Fixpoint &fp2) const {
    CLICK(cond_neq);

    return this->val != fp2.val;
}

// ======================================
//
// Comparisons with zero
//
// ======================================

inline int32_t Fixpoint::gt_zero() const { return (val > 0); }

inline int32_t Fixpoint::ge_zero() const { return (val >= 0); }

inline int32_t Fixpoint::eq_zero() const { return (val == 0); }

inline int32_t Fixpoint::ne_zero() const { return (val != 0); }

inline int32_t Fixpoint::le_zero() const { return (val <= 0); }

inline int32_t Fixpoint::lt_zero() const { return (val < 0); }

// ======================================
//
// Mixed math.
//
// ======================================

inline Fixpoint operator*(int32_t i, Fixpoint const &fp) { return Fixpoint(i) * fp; }
inline Fixpoint operator*(uint32_t i, Fixpoint const &fp) { return Fixpoint(i) * fp; }
inline Fixpoint operator*(double d, Fixpoint const &fp) { return Fixpoint(d) * fp; }

inline Fixpoint operator-(int32_t i, Fixpoint const &fp) { return Fixpoint(i) - fp; }
inline Fixpoint operator-(uint32_t i, Fixpoint const &fp) { return Fixpoint(i) - fp; }
inline Fixpoint operator-(double d, Fixpoint const &fp) { return Fixpoint(d) - fp; }

inline Fixpoint operator+(int32_t i, Fixpoint const &fp) { return Fixpoint(i) + fp; }
inline Fixpoint operator+(uint32_t i, Fixpoint const &fp) { return Fixpoint(i) + fp; }
inline Fixpoint operator+(double d, Fixpoint const &fp) { return Fixpoint(d) + fp; }

inline Fixpoint operator/(int32_t i, Fixpoint const &fp) { return Fixpoint(i) / fp; }
inline Fixpoint operator/(uint32_t i, Fixpoint const &fp) { return Fixpoint(i) / fp; }
inline Fixpoint operator/(double d, Fixpoint const &fp) { return Fixpoint(d) / fp; }

#ifdef BADMIX

inline Fixpoint operator*=(int32_t i, Fixpoint fp) { return Fixpoint(i) *= fp; }
inline Fixpoint operator*=(uint32_t i, Fixpoint fp) { return Fixpoint(i) *= fp; }
inline Fixpoint operator*=(double d, Fixpoint fp) { return Fixpoint(d) *= fp; }

#endif

// ======================================
//
// I/O functions.
//
// ======================================

// ======================================
//
// I/O functions.
//
// ======================================

inline std::ostream &operator<<(std::ostream &os, const Fixpoint &fp) {
    os << fp.to_double();

    return os;
}

inline std::istream &operator>>(std::istream &is, Fixpoint &fp) {
    double temp;

    is >> temp;

    fp = temp;

    return is;
}

// ====================================================
//
// Math functions.
//
// ====================================================

inline Fixpoint mul_div(Fixpoint a, Fixpoint b, Fixpoint c) {
    Fixpoint r;
    r.val = (int32_t)fix_mul_div((fix)a.val, (fix)b.val, (fix)c.val);
    return r;
}

inline Fixpoint sqrt(Fixpoint a) {
    Fixpoint ans;

    ans.val = fix_sqrt(a.val);

    return ans;
}

inline Fixpoint exp(Fixpoint a) {
    Fixpoint ans;
    ans.val = fix_exp(a.val);
    return ans;
}

inline int32_t floor(Fixpoint a) { return a.val >> SHIFTUP; }

inline Fixpoint sin(Fixpoint a) {
    Fixpoint ans;

    ans.val = fix_sin(a.to_fixang());

    return ans;
}

inline Fixpoint cos(Fixpoint a) {
    Fixpoint ans;

    ans.val = fix_cos(a.to_fixang());

    return ans;
}

inline Fixpoint tan(Fixpoint a) {
    Fixpoint sn, cs;

    sn = sin(a);
    cs = cos(a);
    if (cs == 0)
        return 0;
    else
        return sn / cs;
}

inline Fixpoint asin(Fixpoint a) {
    Fixpoint ans;

    ans.fixang_to(fix_asin(a.to_fix()));

    return ans;
}

inline Fixpoint acos(Fixpoint a) {
    Fixpoint ans;

    ans.fixang_to(fix_acos(a.to_fix()));

    return ans;
}

inline void sincos(Fixpoint ang, Fixpoint *sn, Fixpoint *cs) {
    fix fsn, fcs;
    fix_sincos(ang.to_fixang(), &fsn, &fcs);
    sn->val = fsn;
    cs->val = fcs;
}

inline Fixpoint atan2(Fixpoint y, Fixpoint x) {
    Fixpoint ans;

    ans.fixang_to(fix_atan2(y.to_fix(), x.to_fix()));

    return ans;
}

inline Fixpoint fsin(Fixpoint a) {
    Fixpoint ans;

    ans.val = fix_fastsin(a.to_fixang());

    return ans;
}

inline Fixpoint fcos(Fixpoint a) {
    Fixpoint ans;

    ans.val = fix_fastcos(a.to_fixang());

    return ans;
}

inline void fsincos(Fixpoint ang, Fixpoint *sn, Fixpoint *cs) {
    fix fsn, fcs;
    fix_fastsincos(ang.to_fixang(), &fsn, &fcs);
    sn->val = fsn;
    cs->val = fcs;
}

inline Fixpoint abs(Fixpoint fp) {
    Fixpoint ans;

    ans.val = labs(fp.val);

    return ans;
}

#ifdef FIXDEBUG

void touch(Fixpoint &);

#endif /* FIXDEBUG */

#endif /* !__FIXPP_H */
