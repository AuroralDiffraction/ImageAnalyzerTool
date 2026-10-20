#pragma once

#include "core/ElementType.h"
#include "core/Image2D.h"

#include <QByteArray>

namespace iat {

// A stack of 2-D frames exactly as MATLAB holds it: size [rows, cols, frames],
// stored column-major (MATLAB order), values kept in their original storage
// type to save memory. Frames are converted to double only when requested.
//
// The raw buffer is implicitly shared, so copies are cheap and an ImageStack
// can be handed to a worker thread by value.
class ImageStack {
public:
    ImageStack() = default;

    // `data` must hold rows*cols*frames values of `type`, in host byte order,
    // column-major. Returns a null stack if the sizes do not match.
    ImageStack(int rows, int cols, int frames, ElementType type, QByteArray data);

    bool isNull() const { return m_frames == 0; }
    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    int frameCount() const { return m_frames; }
    ElementType elementType() const { return m_type; }
    const QByteArray& rawData() const { return m_data; }

    // Frame `index` (0-based) as a double image: width = cols, height = rows.
    Image2D frame(int index) const;

private:
    int m_rows = 0;
    int m_cols = 0;
    int m_frames = 0;
    ElementType m_type = ElementType::Double;
    QByteArray m_data;
};

} // namespace iat
