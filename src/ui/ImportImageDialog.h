#pragma once

#include "core/Image2D.h"

#include <QDialog>
#include <QList>
#include <QString>

#include <functional>

class QComboBox;
class QLabel;

namespace iat {

// An image another tab can give the hyperspectral workspace: the single
// image, the light or dark frames on display, or the calibrated image.
struct ImportCandidate {
    QString tabName;    // "Session 1"
    QString imageName;  // "Image", "Calibrated frames", ...
    QString info;       // "frame 1 of 8 · 1024 × 1280"
    Image2D image;      // the values (not the 8-bit display)
};

// Lists the images of every open tab.
using ImportProvider = std::function<QList<ImportCandidate>()>;

// "Import from workspace": choose a tab, then one of its images, with a
// preview of the chosen image.
class ImportImageDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImportImageDialog(const QList<ImportCandidate>& candidates, QWidget* parent = nullptr);

    // The chosen candidate, or -1.
    int selectedIndex() const;
    // Chooses the first candidate of `tabName` named `imageName` (for tests).
    bool select(const QString& tabName, const QString& imageName);

private:
    void fillImages();
    void showPreview();

    QList<ImportCandidate> m_candidates;
    QComboBox* m_tab = nullptr;
    QComboBox* m_image = nullptr;
    QLabel* m_preview = nullptr;
    QLabel* m_info = nullptr;
};

} // namespace iat
