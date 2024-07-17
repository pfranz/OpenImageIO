// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

// clang-format off

/// \file
/// The TypeDesc class is used to describe simple data types.


#pragma once

#if defined(_MSC_VER)
// Ignore warnings about conditional expressions that always evaluate true
// on a given platform but may evaluate differently on another. There's
// nothing wrong with such conditionals.
#    pragma warning(disable : 4127)
#endif

#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/string_view.h>
#include <OpenImageIO/strutil.h>

// Define symbols that let client applications determine if newly added
// features are supported.
#define OIIO_TYPEDESC_VECTOR2 1



OIIO_NAMESPACE_BEGIN

/////////////////////////////////////////////////////////////////////////////
/// A TypeDesc describes simple data types.
///
/// It frequently comes up (in my experience, with renderers and image
/// handling programs) that you want a way to describe data that is passed
/// through APIs through blind pointers.  These are some simple classes
/// that provide a simple type descriptor system.  This is not meant to
/// be comprehensive -- for example, there is no provision for structs,
/// unions, pointers, const, or 'nested' type definitions.  Just simple
/// integer and floating point, *common* aggregates such as 3-points,
/// and reasonably-lengthed arrays thereof.
///
/////////////////////////////////////////////////////////////////////////////

struct OIIO_UTIL_API TypeDesc {
    /// BASETYPE is a simple enum describing the base data types that
    /// correspond (mostly) to the C/C++ built-in types.
    enum BASETYPE {
        UNKNOWN,            ///< unknown type
        NONE,               ///< void/no type
        UINT8,              ///< 8-bit unsigned int values ranging from 0..255,
                            ///<   (C/C++ `unsigned char`).
        UCHAR=UINT8,
        INT8,               ///< 8-bit int values ranging from -128..127,
                            ///<   (C/C++ `char`).
        CHAR=INT8,
        UINT16,             ///< 16-bit int values ranging from 0..65535,
                            ///<   (C/C++ `unsigned short`).
        USHORT=UINT16,
        INT16,              ///< 16-bit int values ranging from -32768..32767,
                            ///<   (C/C++ `short`).
        SHORT=INT16,
        UINT32,             ///< 32-bit unsigned int values (C/C++ `unsigned int`).
        UINT=UINT32,
        INT32,              ///< signed 32-bit int values (C/C++ `int`).
        INT=INT32,
        UINT64,             ///< 64-bit unsigned int values (C/C++
                            ///<   `unsigned long long` on most architectures).
        ULONGLONG=UINT64,
        INT64,              ///< signed 64-bit int values (C/C++ `long long`
                            ///<   on most architectures).
        LONGLONG=INT64,
        HALF,               ///< 16-bit IEEE floating point values (OpenEXR `half`).
        FLOAT,              ///< 32-bit IEEE floating point values, (C/C++ `float`).
        DOUBLE,             ///< 64-bit IEEE floating point values, (C/C++ `double`).
        STRING,             ///< Character string.
        PTR,                ///< A pointer value.
        USTRINGHASH,        ///< The hash of a ustring
        LASTBASE
    };

    /// AGGREGATE describes whether our TypeDesc is a simple scalar of one
    /// of the BASETYPE's, or one of several simple aggregates.
    ///
    /// Note that aggregates and arrays are different. A `TypeDesc(FLOAT,3)`
    /// is an array of three floats, a `TypeDesc(FLOAT,VEC3)` is a single
    /// 3-component vector comprised of floats, and `TypeDesc(FLOAT,3,VEC3)`
    /// is an array of 3 vectors, each of which is comprised of 3 floats.
    enum AGGREGATE {
        SCALAR   =  1,  ///< A single scalar value (such as a raw `int` or
                        ///<   `float` in C).  This is the default.
        VEC2     =  2,  ///< 2 values representing a 2D vector.
        VEC3     =  3,  ///< 3 values representing a 3D vector.
        VEC4     =  4,  ///< 4 values representing a 4D vector.
        MATRIX33 =  9,  ///< 9 values representing a 3x3 matrix.
        MATRIX44 = 16   ///< 16 values representing a 4x4 matrix.
    };

    /// VECSEMANTICS gives hints about what the data represent (for example,
    /// if a spatial vector quantity should transform as a point, direction
    /// vector, or surface normal).
    enum VECSEMANTICS {
        NOXFORM     = 0,  ///< No semantic hints.
        NOSEMANTICS = 0,  ///< No semantic hints.
        COLOR,            ///< Color
        POINT,            ///< Point: a spatial location
        VECTOR,           ///< Vector: a spatial direction
        NORMAL,           ///< Normal: a surface normal
        TIMECODE,         ///< indicates an `int[2]` representing the standard
                          ///<   4-byte encoding of an SMPTE timecode.
        KEYCODE,          ///< indicates an `int[7]` representing the standard
                          ///<   28-byte encoding of an SMPTE keycode.
        RATIONAL,         ///< A VEC2 representing a rational number `val[0] / val[1]`
        BOX,              ///< A VEC2[2] or VEC3[2] that represents a 2D or 3D bounds (min/max)
    };

    unsigned char basetype;      ///< C data type at the heart of our type
    unsigned char aggregate;     ///< What kind of AGGREGATE is it?
    unsigned char vecsemantics;  ///< Hint: What does the aggregate represent?
    unsigned char reserved;      ///< Reserved for future expansion
    int arraylen;                ///< Array length, 0 = not array, -1 = unsized

    /// Construct from a BASETYPE and optional aggregateness, semantics,
    /// and arrayness.
    OIIO_HOSTDEVICE constexpr TypeDesc (BASETYPE btype=UNKNOWN, AGGREGATE agg=SCALAR,
                        VECSEMANTICS semantics=NOSEMANTICS,
                        int arraylen=0) noexcept
        : basetype(static_cast<unsigned char>(btype)),
          aggregate(static_cast<unsigned char>(agg)),
          vecsemantics(static_cast<unsigned char>(semantics)), reserved(0),
          arraylen(arraylen)
          { }

    /// Construct an array of a non-aggregate BASETYPE.
    OIIO_HOSTDEVICE constexpr TypeDesc (BASETYPE btype, int arraylen) noexcept
        : TypeDesc(btype, SCALAR, NOSEMANTICS, arraylen) {}

    /// Construct an array from BASETYPE, AGGREGATE, and array length,
    /// with unspecified (or moot) semantic hints.
    OIIO_HOSTDEVICE constexpr TypeDesc (BASETYPE btype, AGGREGATE agg, int arraylen) noexcept
        : TypeDesc(btype, agg, NOSEMANTICS, arraylen) {}

    /// Construct from a string (e.g., "float[3]").  If no valid
    /// type could be assembled, set base to UNKNOWN.
    ///
    /// Examples:
    /// ```
    ///      TypeDesc("int") == TypeDesc(TypeDesc::INT)            // C++ int32_t
    ///      TypeDesc("float") == TypeDesc(TypeDesc::FLOAT)        // C++ float
    ///      TypeDesc("uint16") == TypeDesc(TypeDesc::UINT16)      // C++ uint16_t
    ///      TypeDesc("float[4]") == TypeDesc(TypeDesc::FLOAT, 4)  // array
    ///      TypeDesc("point") == TypeDesc(TypeDesc::FLOAT,
    ///                                    TypeDesc::VEC3, TypeDesc::POINT)
    /// ```
    ///
    TypeDesc (string_view typestring);

    /// Copy constructor.
    constexpr TypeDesc (const TypeDesc &t) noexcept = default;

    /// Return the name, for printing and whatnot.  For example,
    /// "float", "int[5]", "normal"
    const char *c_str() const;

    friend std::ostream& operator<< (std::ostream& o, const TypeDesc& t) {
        o << t.c_str();  return o;
    }

    /// Return the number of elements: 1 if not an array, or the array
    /// length. Invalid to call this for arrays of undetermined size.
    OIIO_HOSTDEVICE constexpr size_t numelements () const noexcept {
        OIIO_DASSERT_MSG (arraylen >= 0, "Called numelements() on TypeDesc "
                          "of array with unspecified length (%d)", arraylen);
        return (arraylen >= 1 ? arraylen : 1);
    }

    /// Return the number of basetype values: the aggregate count multiplied
    /// by the array length (or 1 if not an array). Invalid to call this
    /// for arrays of undetermined size.
    OIIO_HOSTDEVICE constexpr size_t basevalues () const noexcept {
        return numelements() * aggregate;
    }

    /// Does this TypeDesc describe an array?
    OIIO_HOSTDEVICE constexpr bool is_array () const noexcept { return (arraylen != 0); }

    /// Does this TypeDesc describe an array, but whose length is not
    /// specified?
    OIIO_HOSTDEVICE constexpr bool is_unsized_array () const noexcept { return (arraylen < 0); }

    /// Does this TypeDesc describe an array, whose length is specified?
    OIIO_HOSTDEVICE constexpr bool is_sized_array () const noexcept { return (arraylen > 0); }

    /// Return the size, in bytes, of this type.
    ///
    OIIO_HOSTDEVICE size_t size () const noexcept {
        OIIO_DASSERT_MSG (arraylen >= 0, "Called size() on TypeDesc "
                          "of array with unspecified length (%d)", arraylen);
        size_t a = (size_t) (arraylen > 0 ? arraylen : 1);
        if (sizeof(size_t) > sizeof(int)) {
            // size_t has plenty of room for this multiplication
            return a * elementsize();
        } else {
            // need overflow protection
            unsigned long long s = (unsigned long long) a * elementsize();
            const size_t toobig = std::numeric_limits<size_t>::max();
            return s < toobig ? (size_t)s : toobig;
        }
    }

    /// Return the type of one element, i.e., strip out the array-ness.
    ///
    OIIO_HOSTDEVICE constexpr TypeDesc elementtype () const noexcept {
        TypeDesc t (*this);  t.arraylen = 0;  return t;
    }

    /// Return the size, in bytes, of one element of this type (that is,
    /// ignoring whether it's an array).
    OIIO_HOSTDEVICE size_t elementsize () const noexcept { return aggregate * basesize(); }

    /// Return just the underlying C scalar type, i.e., strip out the
    /// array-ness and the aggregateness.
    OIIO_HOSTDEVICE constexpr TypeDesc scalartype() const { return TypeDesc(BASETYPE(basetype)); }

    /// Return the base type size, i.e., stripped of both array-ness
    /// and aggregateness.
    size_t basesize () const noexcept;

    /// True if it's a floating-point type (versus a fundamentally
    /// integral type or something else like a string).
    bool is_floating_point () const noexcept;

    /// True if it's a signed type that allows for negative values.
    bool is_signed () const noexcept;

    /// Shortcut: is it UNKNOWN?
    OIIO_HOSTDEVICE constexpr bool is_unknown () const noexcept { return (basetype == UNKNOWN); }

    /// if (typedesc) is the same as asking whether it's not UNKNOWN.
    OIIO_HOSTDEVICE constexpr operator bool () const noexcept { return (basetype != UNKNOWN); }

    /// Set *this to the type described in the string.  Return the
    /// length of the part of the string that describes the type.  If
    /// no valid type could be assembled, return 0 and do not modify
    /// *this.
    size_t fromstring (string_view typestring);

    /// Compare two TypeDesc values for equality.
    ///
    OIIO_HOSTDEVICE constexpr bool operator== (const TypeDesc &t) const noexcept {
        return basetype == t.basetype && aggregate == t.aggregate &&
            vecsemantics == t.vecsemantics && arraylen == t.arraylen;
    }

    /// Compare two TypeDesc values for inequality.
    ///
    OIIO_HOSTDEVICE constexpr bool operator!= (const TypeDesc &t) const noexcept { return ! (*this == t); }

    /// Compare a TypeDesc to a basetype (it's the same if it has the
    /// same base type and is not an aggregate or an array).
    OIIO_HOSTDEVICE friend constexpr bool operator== (const TypeDesc &t, BASETYPE b) noexcept {
        return (BASETYPE)t.basetype == b && (AGGREGATE)t.aggregate == SCALAR && !t.is_array();
    }
    OIIO_HOSTDEVICE friend constexpr bool operator== (BASETYPE b, const TypeDesc &t) noexcept {
        return (BASETYPE)t.basetype == b && (AGGREGATE)t.aggregate == SCALAR && !t.is_array();
    }

    /// Compare a TypeDesc to a basetype (it's the same if it has the
    /// same base type and is not an aggregate or an array).
    OIIO_HOSTDEVICE friend constexpr bool operator!= (const TypeDesc &t, BASETYPE b) noexcept {
        return (BASETYPE)t.basetype != b || (AGGREGATE)t.aggregate != SCALAR || t.is_array();
    }
    OIIO_HOSTDEVICE friend constexpr bool operator!= (BASETYPE b, const TypeDesc &t) noexcept {
        return (BASETYPE)t.basetype != b || (AGGREGATE)t.aggregate != SCALAR || t.is_array();
    }

    /// TypeDesc's are equivalent if they are equal, or if their only
    /// inequality is differing vector semantics.
    OIIO_HOSTDEVICE friend constexpr bool equivalent (const TypeDesc &a, const TypeDesc &b) noexcept {
        return a.basetype == b.basetype && a.aggregate == b.aggregate &&
               (a.arraylen == b.arraylen || (a.is_unsized_array() && b.is_sized_array())
                                         || (a.is_sized_array()   && b.is_unsized_array()));
    }
    /// Member version of equivalent
    OIIO_HOSTDEVICE constexpr bool equivalent (const TypeDesc &b) const noexcept {
        return this->basetype == b.basetype && this->aggregate == b.aggregate &&
               (this->arraylen == b.arraylen || (this->is_unsized_array() && b.is_sized_array())
                                             || (this->is_sized_array()   && b.is_unsized_array()));
    }

    /// Is this a 2-vector aggregate (of the given type, float by default)?
    OIIO_HOSTDEVICE constexpr bool is_vec2 (BASETYPE b=FLOAT) const noexcept {
        return this->aggregate == VEC2 && this->basetype == b && !is_array();
    }

    /// Is this a 3-vector aggregate (of the given type, float by default)?
    OIIO_HOSTDEVICE constexpr bool is_vec3 (BASETYPE b=FLOAT) const noexcept {
        return this->aggregate == VEC3 && this->basetype == b && !is_array();
    }

    /// Is this a 4-vector aggregate (of the given type, float by default)?
    OIIO_HOSTDEVICE constexpr bool is_vec4 (BASETYPE b=FLOAT) const noexcept {
        return this->aggregate == VEC4 && this->basetype == b && !is_array();
    }

    /// Is this an array of aggregates that represents a 2D bounding box?
    OIIO_HOSTDEVICE constexpr bool is_box2 (BASETYPE b=FLOAT) const noexcept {
        return this->aggregate == VEC2 && this->basetype == b && arraylen == 2
                && this->vecsemantics == BOX;
    }

    /// Is this an array of aggregates that represents a 3D bounding box?
    OIIO_HOSTDEVICE constexpr bool is_box3 (BASETYPE b=FLOAT) const noexcept {
        return this->aggregate == VEC3 && this->basetype == b && arraylen == 2
                && this->vecsemantics == BOX;
    }

    /// Demote the type to a non-array
    ///
    OIIO_HOSTDEVICE void unarray (void) noexcept { arraylen = 0; }

    /// Test for lexicographic 'less', comes in handy for lots of STL
    /// containers and algorithms.
    bool operator< (const TypeDesc &x) const noexcept;

    /// Given base data types of a and b, return a basetype that is a best
    /// guess for one that can handle both without any loss of range or
    /// precision.
    static BASETYPE basetype_merge(TypeDesc a, TypeDesc b);
    static BASETYPE basetype_merge(TypeDesc a, TypeDesc b, TypeDesc c) {
        return basetype_merge(basetype_merge(a, b), c);
    }

#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(1,8,0) && OIIO_VERSION_LESS(2,7,0) && !defined(OIIO_DOXYGEN)
    // DEPRECATED(1.8): These static const member functions were mildly
    // problematic because they required external linkage (and possibly
    // even static initialization order fiasco) and were a memory reference
    // that incurred some performance penalty and inability to optimize.
    // Please instead use the out-of-class constexpr versions below.  We
    // will eventually remove these.
#ifdef __INTEL_COMPILER
#    define OIIO_DEPRECATED_TYPEDESC_STATICS
#else
#    define OIIO_DEPRECATED_TYPEDESC_STATICS \
        OIIO_DEPRECATED("Use the version that takes a tostring_formatting struct (1.8)")
#endif
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeFloat;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeColor;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeString;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeInt;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeHalf;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypePoint;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeVector;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeNormal;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeMatrix;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeMatrix33;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeMatrix44;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeTimeCode;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeKeyCode;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeFloat4;
    OIIO_DEPRECATED_TYPEDESC_STATICS static const TypeDesc TypeRational;
#endif
#undef OIIO_DEPRECATED_TYPEDESC_STATICS
};

// Validate that TypeDesc can be used directly as POD in a C interface.
static_assert(std::is_default_constructible<TypeDesc>(), "TypeDesc is not default constructable.");
static_assert(std::is_trivially_copyable<TypeDesc>(), "TypeDesc is not trivially copyable.");
static_assert(std::is_trivially_destructible<TypeDesc>(), "TypeDesc is not trivially destructible.");
static_assert(std::is_trivially_move_constructible<TypeDesc>(), "TypeDesc is not move constructible.");
static_assert(std::is_trivially_copy_constructible<TypeDesc>(), "TypeDesc is not copy constructible.");
static_assert(std::is_trivially_move_assignable<TypeDesc>(), "TypeDesc is not move assignable.");

// Static values for commonly used types. Because these are constexpr,
// they should incur no runtime construction cost and should optimize nicely
// in various ways.
OIIO_INLINE_CONSTEXPR TypeDesc TypeUnknown (TypeDesc::UNKNOWN);
OIIO_INLINE_CONSTEXPR TypeDesc TypeFloat (TypeDesc::FLOAT);
OIIO_INLINE_CONSTEXPR TypeDesc TypeColor (TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::COLOR);
OIIO_INLINE_CONSTEXPR TypeDesc TypePoint (TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::POINT);
OIIO_INLINE_CONSTEXPR TypeDesc TypeVector (TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::VECTOR);
OIIO_INLINE_CONSTEXPR TypeDesc TypeNormal (TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::NORMAL);
OIIO_INLINE_CONSTEXPR TypeDesc TypeMatrix33 (TypeDesc::FLOAT, TypeDesc::MATRIX33);
OIIO_INLINE_CONSTEXPR TypeDesc TypeMatrix44 (TypeDesc::FLOAT, TypeDesc::MATRIX44);
OIIO_INLINE_CONSTEXPR TypeDesc TypeMatrix = TypeMatrix44;
OIIO_INLINE_CONSTEXPR TypeDesc TypeFloat2 (TypeDesc::FLOAT, TypeDesc::VEC2);
OIIO_INLINE_CONSTEXPR TypeDesc TypeVector2 (TypeDesc::FLOAT, TypeDesc::VEC2, TypeDesc::VECTOR);
OIIO_INLINE_CONSTEXPR TypeDesc TypeFloat4 (TypeDesc::FLOAT, TypeDesc::VEC4);
OIIO_INLINE_CONSTEXPR TypeDesc TypeVector4 = TypeFloat4;
OIIO_INLINE_CONSTEXPR TypeDesc TypeString (TypeDesc::STRING);
OIIO_INLINE_CONSTEXPR TypeDesc TypeInt (TypeDesc::INT);
OIIO_INLINE_CONSTEXPR TypeDesc TypeUInt (TypeDesc::UINT);
OIIO_INLINE_CONSTEXPR TypeDesc TypeInt32 (TypeDesc::INT);
OIIO_INLINE_CONSTEXPR TypeDesc TypeUInt32 (TypeDesc::UINT);
OIIO_INLINE_CONSTEXPR TypeDesc TypeInt16 (TypeDesc::INT16);
OIIO_INLINE_CONSTEXPR TypeDesc TypeUInt16 (TypeDesc::UINT16);
OIIO_INLINE_CONSTEXPR TypeDesc TypeInt8 (TypeDesc::INT8);
OIIO_INLINE_CONSTEXPR TypeDesc TypeUInt8 (TypeDesc::UINT8);
OIIO_INLINE_CONSTEXPR TypeDesc TypeInt64 (TypeDesc::INT64);
OIIO_INLINE_CONSTEXPR TypeDesc TypeUInt64 (TypeDesc::UINT64);
OIIO_INLINE_CONSTEXPR TypeDesc TypeVector2i(TypeDesc::INT, TypeDesc::VEC2);
OIIO_INLINE_CONSTEXPR TypeDesc TypeVector3i(TypeDesc::INT, TypeDesc::VEC3);
OIIO_INLINE_CONSTEXPR TypeDesc TypeBox2(TypeDesc::FLOAT, TypeDesc::VEC2, TypeDesc::BOX, 2);
OIIO_INLINE_CONSTEXPR TypeDesc TypeBox3(TypeDesc::FLOAT, TypeDesc::VEC3, TypeDesc::BOX, 2);
OIIO_INLINE_CONSTEXPR TypeDesc TypeBox2i(TypeDesc::INT, TypeDesc::VEC2, TypeDesc::BOX, 2);
OIIO_INLINE_CONSTEXPR TypeDesc TypeBox3i(TypeDesc::INT, TypeDesc::VEC3, TypeDesc::BOX, 2);
OIIO_INLINE_CONSTEXPR TypeDesc TypeHalf (TypeDesc::HALF);
OIIO_INLINE_CONSTEXPR TypeDesc TypeTimeCode (TypeDesc::UINT, TypeDesc::SCALAR, TypeDesc::TIMECODE, 2);
OIIO_INLINE_CONSTEXPR TypeDesc TypeKeyCode (TypeDesc::INT, TypeDesc::SCALAR, TypeDesc::KEYCODE, 7);
OIIO_INLINE_CONSTEXPR TypeDesc TypeRational(TypeDesc::INT, TypeDesc::VEC2, TypeDesc::RATIONAL);
OIIO_INLINE_CONSTEXPR TypeDesc TypePointer(TypeDesc::PTR);
OIIO_INLINE_CONSTEXPR TypeDesc TypeUstringhash(TypeDesc::USTRINGHASH);



#if OIIO_DISABLE_DEPRECATED < OIIO_MAKE_VERSION(2,1,0) && OIIO_VERSION_LESS(2,7,0)
// DEPRECATED(2.1)
OIIO_DEPRECATED("Use the version that takes a tostring_formatting struct")
OIIO_UTIL_API
std::string tostring (TypeDesc type, const void *data,
                      const char *float_fmt,                // E.g. "%g"
                      const char *string_fmt = "%s",        // E.g. "\"%s\""
                      const char aggregate_delim[2] = "()", // Both sides of vector
                      const char *aggregate_sep = ",",      // E.g. ", "
                      const char array_delim[2] = "{}",     // Both sides of array
                      const char *array_sep = ",");         // E.g. "; "
#endif


/// A template mechanism for getting the a base type from C type
///
template<typename T> struct BaseTypeFromC {};
template<> struct BaseTypeFromC<unsigned char> { static const TypeDesc::BASETYPE value = TypeDesc::UINT8; };
template<> struct BaseTypeFromC<char> { static const TypeDesc::BASETYPE value = TypeDesc::INT8; };
template<> struct BaseTypeFromC<unsigned short> { static const TypeDesc::BASETYPE value = TypeDesc::UINT16; };
template<> struct BaseTypeFromC<short> { static const TypeDesc::BASETYPE value = TypeDesc::INT16; };
template<> struct BaseTypeFromC<unsigned int> { static const TypeDesc::BASETYPE value = TypeDesc::UINT; };
template<> struct BaseTypeFromC<int> { static const TypeDesc::BASETYPE value = TypeDesc::INT; };
template<> struct BaseTypeFromC<unsigned long long> { static const TypeDesc::BASETYPE value = TypeDesc::UINT64; };
template<> struct BaseTypeFromC<long long> { static const TypeDesc::BASETYPE value = TypeDesc::INT64; };
#if defined(_HALF_H_) || defined(IMATH_HALF_H_)
template<> struct BaseTypeFromC<half> { static const TypeDesc::BASETYPE value = TypeDesc::HALF; };
#endif
template<> struct BaseTypeFromC<float> { static const TypeDesc::BASETYPE value = TypeDesc::FLOAT; };
template<> struct BaseTypeFromC<double> { static const TypeDesc::BASETYPE value = TypeDesc::DOUBLE; };
template<> struct BaseTypeFromC<const char*> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
template<> struct BaseTypeFromC<char*> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
template<> struct BaseTypeFromC<std::string> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
template<> struct BaseTypeFromC<string_view> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
class ustring;
template<> struct BaseTypeFromC<ustring> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
template<size_t S> struct BaseTypeFromC<char[S]> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
template<size_t S> struct BaseTypeFromC<const char[S]> { static const TypeDesc::BASETYPE value = TypeDesc::STRING; };
template<typename P> struct BaseTypeFromC<P*> { static const TypeDesc::BASETYPE value = TypeDesc::PTR; };

/// A template mechanism for getting the TypeDesc from a C type.
/// The default for simple types is just the TypeDesc based on BaseTypeFromC.
/// But we can specialize more complex types.
template<typename T> struct TypeDescFromC { static const constexpr TypeDesc value() { return TypeDesc(BaseTypeFromC<T>::value); } };
template<> struct TypeDescFromC<int32_t> { static const constexpr TypeDesc value() { return TypeDesc::INT32; } };
template<> struct TypeDescFromC<uint32_t> { static const constexpr TypeDesc value() { return TypeDesc::UINT32; } };
template<> struct TypeDescFromC<int16_t> { static const constexpr TypeDesc value() { return TypeDesc::INT16; } };
template<> struct TypeDescFromC<uint16_t> { static const constexpr TypeDesc value() { return TypeDesc::UINT16; } };
template<> struct TypeDescFromC<int8_t> { static const constexpr TypeDesc value() { return TypeDesc::INT8; } };
template<> struct TypeDescFromC<uint8_t> { static const constexpr TypeDesc value() { return TypeDesc::UINT8; } };
template<> struct TypeDescFromC<float> { static const constexpr TypeDesc value() { return TypeDesc::FLOAT; } };
#if defined(_HALF_H_) || defined(IMATH_HALF_H_)
template<> struct TypeDescFromC<half> { static const constexpr TypeDesc value() { return TypeDesc::HALF; } };
#endif
template<> struct TypeDescFromC<double> { static const constexpr TypeDesc value() { return TypeDesc::DOUBLE; } };
template<> struct TypeDescFromC<char*> { static const constexpr TypeDesc value() { return TypeDesc::STRING; } };
template<> struct TypeDescFromC<const char*> { static const constexpr TypeDesc value() { return TypeDesc::STRING; } };
template<size_t S> struct TypeDescFromC<char[S]> { static const constexpr TypeDesc value() { return TypeDesc::STRING; } };
template<size_t S> struct TypeDescFromC<const char[S]> { static const constexpr TypeDesc value() { return TypeDesc::STRING; } };
template<> struct TypeDescFromC<ustring> { static const constexpr TypeDesc value() { return TypeDesc::STRING; } };
template<typename T> struct TypeDescFromC<T*> { static const constexpr TypeDesc value() { return TypeDesc::PTR; } };
#ifdef INCLUDED_IMATHVEC_H
template<> struct TypeDescFromC<Imath::V3f> { static const constexpr TypeDesc value() { return TypeVector; } };
template<> struct TypeDescFromC<Imath::V2f> { static const constexpr TypeDesc value() { return TypeVector2; } };
template<> struct TypeDescFromC<Imath::V4f> { static const constexpr TypeDesc value() { return TypeVector4; } };
template<> struct TypeDescFromC<Imath::V2i> { static const constexpr TypeDesc value() { return TypeVector2i; } };
template<> struct TypeDescFromC<Imath::V3i> { static const constexpr TypeDesc value() { return TypeVector3i; } };
#endif
#ifdef INCLUDED_IMATHCOLOR_H
template<> struct TypeDescFromC<Imath::Color3f> { static const constexpr TypeDesc value() { return TypeColor; } };
#endif
#ifdef INCLUDED_IMATHMATRIX_H
template<> struct TypeDescFromC<Imath::M33f> { static const constexpr TypeDesc value() { return TypeMatrix33; } };
template<> struct TypeDescFromC<Imath::M44f> { static const constexpr TypeDesc value() { return TypeMatrix44; } };
template<> struct TypeDescFromC<Imath::M33d> { static const constexpr TypeDesc value() { return TypeDesc(TypeDesc::DOUBLE, TypeDesc::MATRIX33); } };
template<> struct TypeDescFromC<Imath::M44d> { static const constexpr TypeDesc value() { return TypeDesc(TypeDesc::DOUBLE, TypeDesc::MATRIX44); } };
#endif
#ifdef INCLUDED_IMATHBOX_H
template<> struct TypeDescFromC<Imath::Box2f> { static const constexpr TypeDesc value() { return TypeBox2; } };
template<> struct TypeDescFromC<Imath::Box2i> { static const constexpr TypeDesc value() { return TypeBox2i; } };
template<> struct TypeDescFromC<Imath::Box3f> { static const constexpr TypeDesc value() { return TypeBox3; } };
template<> struct TypeDescFromC<Imath::Box3i> { static const constexpr TypeDesc value() { return TypeBox3i; } };
#endif


class ustringhash;  // forward declaration



/// A template mechanism for getting C type of TypeDesc::BASETYPE.
///
template<int b> struct CType {};
template<> struct CType<(int)TypeDesc::UINT8> { typedef unsigned char type; };
template<> struct CType<(int)TypeDesc::INT8> { typedef char type; };
template<> struct CType<(int)TypeDesc::UINT16> { typedef unsigned short type; };
template<> struct CType<(int)TypeDesc::INT16> { typedef short type; };
template<> struct CType<(int)TypeDesc::UINT> { typedef unsigned int type; };
template<> struct CType<(int)TypeDesc::INT> { typedef int type; };
template<> struct CType<(int)TypeDesc::UINT64> { typedef unsigned long long type; };
template<> struct CType<(int)TypeDesc::INT64> { typedef long long type; };
#if defined(_HALF_H_) || defined(IMATH_HALF_H_)
template<> struct CType<(int)TypeDesc::HALF> { typedef half type; };
#endif
template<> struct CType<(int)TypeDesc::FLOAT> { typedef float type; };
template<> struct CType<(int)TypeDesc::DOUBLE> { typedef double type; };
template<> struct CType<(int)TypeDesc::USTRINGHASH> { typedef ustringhash type; };



/// Helper class for tostring() that contains a whole bunch of parameters
/// that control exactly how all the data types that can be described as
/// TypeDesc ought to be formatted as a string. Uses printf-like
/// conventions. This will someday be deprecated.
struct OIIO_UTIL_API tostring_formatting {
    // Printf-like formatting specs for int, float, string, pointer data.
    const char *int_fmt = "%d";
    const char *float_fmt = "%g";
    const char *string_fmt = "\"%s\"";
    const char *ptr_fmt = "%p";
    // Aggregates are multi-part types, like VEC3, etc. How do we mark the
    // start, separation between elements, and end?
    const char *aggregate_begin = "(";
    const char *aggregate_end = ")";
    const char *aggregate_sep = ",";
    // For arrays, how do we mark the start, separation between elements,
    // and end?
    const char *array_begin = "{";
    const char *array_end = "}";
    const char *array_sep = ",";
    // Miscellaneous control flags, OR the enum values together.
    enum Flags { None=0, escape_strings=1, quote_single_string=2 };
    int flags = escape_strings;
    // Reserved space for future expansion without breaking the ABI.
    const char *uint_fmt = "%u";
    const char *reserved2 = "";
    const char *reserved3 = "";
    bool use_sprintf = true;

    enum Notation { STDFORMAT };

    tostring_formatting() = default;
    tostring_formatting(const char *int_fmt, const char *float_fmt = "%g",
        const char *string_fmt = "\"%s\"", const char *ptr_fmt = "%p",
        const char *aggregate_begin = "(", const char *aggregate_end = ")",
        const char *aggregate_sep = ",", const char *array_begin = "{",
        const char *array_end = "}", const char *array_sep = ",",
        int flags = escape_strings,
        const char *uint_fmt = "%u");

    // Alternative ctr for std::format notation. You must pass STDFORMAT
    // as the first argument.
    tostring_formatting(Notation notation,
        const char *int_fmt = "{}", const char *uint_fmt = "{}",
        const char *float_fmt = "{}",
        const char *string_fmt = "\"{}\"", const char *ptr_fmt = "{}",
        const char *aggregate_begin = "(", const char *aggregate_end = ")",
        const char *aggregate_sep = ",", const char *array_begin = "{",
        const char *array_end = "}", const char *array_sep = ",",
        int flags = escape_strings);
};



/// Return a string containing the data values formatted according
/// to the type and the optional formatting control arguments. Will be
/// deprecated someday as printf formatting falls out of favor.
OIIO_UTIL_API std::string
tostring(TypeDesc type, const void* data, const tostring_formatting& fmt = {});



/// Given data pointed to by src and described by srctype, copy it to the
/// memory pointed to by dst and described by dsttype, and return true if a
/// conversion is possible, false if it is not. If the types are equivalent,
/// this is a straightforward memory copy. If the types differ, there are
/// several non-equivalent type conversions that will nonetheless succeed:
/// * If dsttype is a string (and therefore dst points to a ustring or a
///   `char*`): it will always succeed, producing a string akin to calling
///   `tostring()`.
/// * If dsttype is int32 or uint32: other integer types will do their best
///   (caveat emptor if you mix signed/unsigned). Also a source string will
///   convert to int if and only if its characters form a valid integer.
/// * If dsttype is float: inteegers and other float types will do
///   their best conversion; strings will convert if and only if their
///   characters form a valid float number.
OIIO_UTIL_API bool
convert_type(TypeDesc srctype, const void* src,
             TypeDesc dsttype, void* dst, int n = 1);


OIIO_NAMESPACE_END



// Supply a fmtlib compatible custom formatter for TypeDesc.
#if FMT_VERSION >= 100000
FMT_BEGIN_NAMESPACE
template<> struct formatter<OIIO::TypeDesc> : ostream_formatter {};
FMT_END_NAMESPACE
#else
FMT_BEGIN_NAMESPACE
template <>
struct formatter<OIIO::TypeDesc> {
    // Parses format specification
    // C++14: constexpr auto parse(format_parse_context& ctx) const {
    auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) // c++11
    {
        // Get the presentation type, if any. Required to be 's'.
        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 's')) ++it;
        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");
        // Return an iterator past the end of the parsed range:
        return it;
    }

    template <typename FormatContext>
    auto format(const OIIO::TypeDesc& t, FormatContext& ctx) OIIO_FMT_CUSTOM_FORMATTER_CONST
    {
        // C++14:   auto format(const OIIO::TypeDesc& p, FormatContext& ctx) const {
        // ctx.out() is an output iterator to write to.
        return format_to(ctx.out(), "{}", t.c_str());
    }
};
FMT_END_NAMESPACE
#endif
