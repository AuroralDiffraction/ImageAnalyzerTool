#pragma once

#include <QString>
#include <QtGlobal>

#include <cstddef>
#include <cstring>

namespace iat {

// Numeric type of the values as they are stored in memory or in a file.
// Note that this can be narrower than the MATLAB class: MATLAB saves a
// double array that only holds whole numbers as uint16, for example.
enum class ElementType { Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Single, Double };

std::size_t elementSize(ElementType type);
QString elementTypeName(ElementType type);

// Reads one value of type T from a possibly unaligned address.
template <typename T>
inline T loadAs(const char* p)
{
    T value;
    std::memcpy(&value, p, sizeof(T));
    return value;
}

// Calls f(T{}) with the C++ type that matches `type`, so a single generic
// lambda can handle every storage type.
template <typename F>
decltype(auto) dispatchElementType(ElementType type, F&& f)
{
    switch (type) {
    case ElementType::Int8:   return f(qint8{});
    case ElementType::UInt8:  return f(quint8{});
    case ElementType::Int16:  return f(qint16{});
    case ElementType::UInt16: return f(quint16{});
    case ElementType::Int32:  return f(qint32{});
    case ElementType::UInt32: return f(quint32{});
    case ElementType::Int64:  return f(qint64{});
    case ElementType::UInt64: return f(quint64{});
    case ElementType::Single: return f(float{});
    case ElementType::Double: break;
    }
    return f(double{});
}

} // namespace iat
