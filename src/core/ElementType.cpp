#include "core/ElementType.h"

namespace iat {

std::size_t elementSize(ElementType type)
{
    return dispatchElementType(type, [](auto tag) { return sizeof(tag); });
}

QString elementTypeName(ElementType type)
{
    switch (type) {
    case ElementType::Int8:   return QStringLiteral("int8");
    case ElementType::UInt8:  return QStringLiteral("uint8");
    case ElementType::Int16:  return QStringLiteral("int16");
    case ElementType::UInt16: return QStringLiteral("uint16");
    case ElementType::Int32:  return QStringLiteral("int32");
    case ElementType::UInt32: return QStringLiteral("uint32");
    case ElementType::Int64:  return QStringLiteral("int64");
    case ElementType::UInt64: return QStringLiteral("uint64");
    case ElementType::Single: return QStringLiteral("single");
    case ElementType::Double: return QStringLiteral("double");
    }
    return QStringLiteral("unknown");
}

} // namespace iat
