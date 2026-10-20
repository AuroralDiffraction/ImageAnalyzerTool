#include "core/ImageStack.h"

#include <utility>

namespace iat {

ImageStack::ImageStack(int rows, int cols, int frames, ElementType type, QByteArray data)
{
    if (rows <= 0 || cols <= 0 || frames <= 0)
        return;
    const qint64 expected = qint64(rows) * cols * frames * qint64(elementSize(type));
    if (expected != data.size())
        return;

    m_rows = rows;
    m_cols = cols;
    m_frames = frames;
    m_type = type;
    m_data = std::move(data);
}

Image2D ImageStack::frame(int index) const
{
    if (isNull() || index < 0 || index >= m_frames)
        return {};

    Image2D out(m_cols, m_rows);
    double* dst = out.data();
    const qsizetype plane = qsizetype(m_rows) * m_cols;

    dispatchElementType(m_type, [&](auto tag) {
        using T = decltype(tag);
        const char* base = m_data.constData() + qsizetype(sizeof(T)) * plane * index;
        // Column-major source -> row-major destination.
        for (int c = 0; c < m_cols; ++c) {
            const char* col = base + qsizetype(sizeof(T)) * qsizetype(c) * m_rows;
            for (int r = 0; r < m_rows; ++r)
                dst[qsizetype(r) * m_cols + c] = static_cast<double>(loadAs<T>(col + sizeof(T) * r));
        }
    });
    return out;
}

} // namespace iat
