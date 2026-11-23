#pragma once

#include "core/HsiColour.h"
#include "core/HsiCorrection.h"

#include <QDialog>

#include <functional>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QRadioButton;

namespace iat {

class SpectrumPlot;

// Settings of the Correction panel of the hyperspectral workspace.
struct CorrectionSettings {
    bool spectralCorrection = false;
    double targetFwhm = 0.0;          // nm; 0: each channel's measured width
    double regularisation = 0.01;
    QString matrixPath;               // empty: compute from the spectral response
    hsi::SpectralCorrection loadedMatrix;  // the matrix read from matrixPath
    double sceneExposure = 0.0;       // ms; 0: unknown
    double whiteExposure = 0.0;
    double whiteReflectance = 1.0;
};

// "Correction settings": details of the spectral response, the spectral
// correction (computed from the response, or a matrix file from the camera
// maker) with a plot of the channel responses before and after it, the
// exposure times and reflectance of the reference frames, and saving /
// loading a sensor profile.
class CorrectionDialog : public QDialog {
    Q_OBJECT
public:
    CorrectionDialog(const hsi::SpectralResponse& response, const QString& responseText, const CorrectionSettings& settings,
                     const QString& framesText, QWidget* parent = nullptr);

    CorrectionSettings settings() const;
    // The correction the settings give (computed or loaded); invalid if none.
    hsi::SpectralCorrection correction() const;

    // Called by the profile buttons (save gets the dialog's settings).
    std::function<void(const CorrectionSettings&)> onSaveProfile;
    std::function<bool()> onLoadProfile;  // true: a profile was loaded, the dialog closes

    // For tests.
    void setTargetFwhm(double nm);
    void setCorrectionEnabled(bool on);
    QString residualText() const;

private:
    void updatePreview();
    void loadMatrix();

    hsi::SpectralResponse m_response;
    CorrectionSettings m_settings;
    QCheckBox* m_enabled = nullptr;
    QRadioButton* m_compute = nullptr;
    QRadioButton* m_fromFile = nullptr;
    QDoubleSpinBox* m_fwhm = nullptr;
    QDoubleSpinBox* m_reg = nullptr;
    QLabel* m_matrixLabel = nullptr;
    QLabel* m_residual = nullptr;
    QComboBox* m_plotWhat = nullptr;
    SpectrumPlot* m_plot = nullptr;
    QDoubleSpinBox* m_sceneExposure = nullptr;
    QDoubleSpinBox* m_whiteExposure = nullptr;
    QDoubleSpinBox* m_whiteReflectance = nullptr;
};

} // namespace iat
