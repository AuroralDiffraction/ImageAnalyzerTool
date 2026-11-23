#include "ui/CorrectionDialog.h"

#include "core/HsiAnalysis.h"
#include "io/HsiCubeIo.h"
#include "ui/SpectrumPlot.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QScrollArea>
#include <QVBoxLayout>

namespace iat {

using namespace hsi;

CorrectionDialog::CorrectionDialog(const SpectralResponse& response, const QString& responseText,
                                   const CorrectionSettings& settings, const QString& framesText, QWidget* parent)
    : QDialog(parent)
    , m_response(response)
    , m_settings(settings)
{
    setWindowTitle(tr("Correction settings"));
    auto* layout = new QHBoxLayout(this);
    // The settings in a scrolling column, so word-wrapped text never squeezes
    // the fields on a small screen.
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* column = new QWidget(scroll);
    auto* left = new QVBoxLayout(column);
    left->setContentsMargins(0, 0, 4, 0);
    scroll->setWidget(column);
    scroll->setFixedWidth(380);
    layout->addWidget(scroll, 0);

    // Spectral response.
    auto* rg = new QGroupBox(tr("Spectral response"), this);
    auto* rl = new QVBoxLayout(rg);
    auto* rtext = new QLabel(responseText.isEmpty() ? tr("No spectral response loaded.") : responseText, rg);
    rtext->setWordWrap(true);
    rtext->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rl->addWidget(rtext);
    left->addWidget(rg);

    // Spectral correction.
    auto* cg = new QGroupBox(tr("Spectral correction"), this);
    auto* cl = new QVBoxLayout(cg);
    m_enabled = new QCheckBox(tr("Correct the channels"), cg);
    m_enabled->setToolTip(tr("Fabry-Perot channels also respond outside their peak (a broad background shared by all "
                             "channels, harmonics, cross-talk). The correction turns the 16 measured channels into "
                             "bands that respond like narrow filters at the channel peaks."));
    cl->addWidget(m_enabled);
    m_compute = new QRadioButton(tr("Compute from the spectral response"), cg);
    m_fromFile = new QRadioButton(tr("Correction matrix from a file"), cg);
    cl->addWidget(m_compute);
    auto* form = new QFormLayout;
    m_fwhm = new QDoubleSpinBox(cg);
    m_fwhm->setRange(0.0, 60.0);
    m_fwhm->setDecimals(1);
    m_fwhm->setSingleStep(1.0);
    m_fwhm->setSuffix(QStringLiteral(" nm"));
    m_fwhm->setSpecialValueText(tr("measured widths"));
    m_fwhm->setToolTip(tr("Width (FWHM) of the target band of each channel; narrower bands separate wavelengths "
                          "better but amplify noise"));
    m_reg = new QDoubleSpinBox(cg);
    m_reg->setRange(0.0, 1.0);
    m_reg->setDecimals(4);
    m_reg->setSingleStep(0.005);
    m_reg->setToolTip(tr("Regularisation: larger keeps noise low, smaller follows the targets more closely"));
    form->addRow(tr("Target width:"), m_fwhm);
    form->addRow(tr("Regularisation:"), m_reg);
    cl->addLayout(form);
    cl->addWidget(m_fromFile);
    auto* fileRow = new QHBoxLayout;
    m_matrixLabel = new QLabel(cg);
    m_matrixLabel->setWordWrap(true);
    auto* loadMatrix = new QPushButton(tr("Load matrix…"), cg);
    loadMatrix->setToolTip(tr("A V × 16 matrix supplied with the camera (.csv or .mat); with 17 columns the first "
                              "is the band's centre wavelength"));
    fileRow->addWidget(m_matrixLabel, 1);
    fileRow->addWidget(loadMatrix);
    cl->addLayout(fileRow);
    m_residual = new QLabel(cg);
    m_residual->setWordWrap(true);
    m_residual->setForegroundRole(QPalette::PlaceholderText);
    cl->addWidget(m_residual);
    left->addWidget(cg);

    // Reference frames.
    auto* fg = new QGroupBox(tr("Reference frames"), this);
    auto* fl = new QFormLayout(fg);
    auto* ftext = new QLabel(framesText, fg);
    ftext->setWordWrap(true);
    fl->addRow(ftext);
    auto exposureSpin = [fg](double v) {
        auto* s = new QDoubleSpinBox(fg);
        s->setRange(0.0, 100000.0);
        s->setDecimals(3);
        s->setSuffix(QStringLiteral(" ms"));
        s->setSpecialValueText(tr("same"));
        s->setValue(v);
        return s;
    };
    m_sceneExposure = exposureSpin(settings.sceneExposure);
    m_whiteExposure = exposureSpin(settings.whiteExposure);
    m_sceneExposure->setToolTip(tr("Exposure time of the image; with the white frame's, the two are compared per ms"));
    m_whiteReflectance = new QDoubleSpinBox(fg);
    m_whiteReflectance->setRange(0.01, 1.0);
    m_whiteReflectance->setDecimals(3);
    m_whiteReflectance->setSingleStep(0.01);
    m_whiteReflectance->setValue(settings.whiteReflectance);
    m_whiteReflectance->setToolTip(tr("Reflectance of the white reference (e.g. 0.99 for Spectralon)"));
    fl->addRow(tr("Image exposure:"), m_sceneExposure);
    fl->addRow(tr("White exposure:"), m_whiteExposure);
    fl->addRow(tr("White reflectance:"), m_whiteReflectance);
    left->addWidget(fg);
    left->addStretch(1);

    // Right: plot of the channel responses.
    auto* right = new QVBoxLayout;
    layout->addLayout(right, 1);
    m_plotWhat = new QComboBox(this);
    m_plotWhat->addItems({tr("Measured channel responses"), tr("Corrected band responses")});
    right->addWidget(m_plotWhat);
    m_plot = new SpectrumPlot(this);
    m_plot->setShowLegend(false);
    m_plot->setShowMarkers(false);
    m_plot->setYLabel(tr("Response"));
    m_plot->setPlaceholder(tr("Load a spectral response to see the channel responses."));
    m_plot->setMinimumSize(460, 320);
    right->addWidget(m_plot, 1);

    auto* bottom = new QHBoxLayout;
    auto* saveProfile = new QPushButton(tr("Save sensor profile…"), this);
    saveProfile->setToolTip(tr("Mosaic layout, bit depth, these settings and the paths of the spectral response, "
                               "correction matrix and reference frames, in one file"));
    auto* loadProfile = new QPushButton(tr("Load sensor profile…"), this);
    bottom->addWidget(saveProfile);
    bottom->addWidget(loadProfile);
    bottom->addStretch(1);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    bottom->addWidget(buttons);
    right->addLayout(bottom);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(loadMatrix, &QPushButton::clicked, this, &CorrectionDialog::loadMatrix);
    connect(saveProfile, &QPushButton::clicked, this, [this] {
        if (onSaveProfile)
            onSaveProfile(this->settings());
    });
    connect(loadProfile, &QPushButton::clicked, this, [this] {
        if (onLoadProfile && onLoadProfile())
            reject();  // the profile replaced the settings already
    });

    m_enabled->setChecked(settings.spectralCorrection);
    m_fwhm->setValue(settings.targetFwhm);
    m_reg->setValue(settings.regularisation);
    (settings.matrixPath.isEmpty() ? m_compute : m_fromFile)->setChecked(true);
    m_matrixLabel->setText(settings.matrixPath.isEmpty() ? tr("No matrix file.") : QFileInfo(settings.matrixPath).fileName());
    m_plotWhat->setCurrentIndex(settings.spectralCorrection ? 1 : 0);
    for (QWidget* w : {static_cast<QWidget*>(m_fwhm), static_cast<QWidget*>(m_reg)})
        w->setEnabled(!response.isNull());
    m_compute->setEnabled(!response.isNull());
    if (response.isNull())
        m_fromFile->setChecked(true);

    auto refresh = [this] { updatePreview(); };
    connect(m_enabled, &QCheckBox::toggled, this, refresh);
    connect(m_compute, &QRadioButton::toggled, this, refresh);
    connect(m_fwhm, &QDoubleSpinBox::valueChanged, this, refresh);
    connect(m_reg, &QDoubleSpinBox::valueChanged, this, refresh);
    connect(m_plotWhat, &QComboBox::currentIndexChanged, this, refresh);
    updatePreview();
    resize(1000, 680);
}

CorrectionSettings CorrectionDialog::settings() const
{
    CorrectionSettings s = m_settings;
    s.spectralCorrection = m_enabled->isChecked();
    s.targetFwhm = m_fwhm->value();
    s.regularisation = m_reg->value();
    if (m_compute->isChecked()) {
        s.matrixPath.clear();
        s.loadedMatrix = SpectralCorrection();
    }
    s.sceneExposure = m_sceneExposure->value();
    s.whiteExposure = m_whiteExposure->value();
    s.whiteReflectance = m_whiteReflectance->value();
    return s;
}

SpectralCorrection CorrectionDialog::correction() const
{
    if (m_fromFile->isChecked())
        return m_settings.loadedMatrix;
    if (m_response.isNull())
        return {};
    return computeCorrection(m_response, m_fwhm->value(), m_reg->value());
}

void CorrectionDialog::setTargetFwhm(double nm)
{
    m_compute->setChecked(true);
    m_fwhm->setValue(nm);
}

void CorrectionDialog::setCorrectionEnabled(bool on)
{
    m_enabled->setChecked(on);
}

QString CorrectionDialog::residualText() const
{
    return m_residual->text();
}

void CorrectionDialog::loadMatrix()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Load correction matrix"),
                                                      QSettings().value(QStringLiteral("hsi/lastDir")).toString(),
                                                      tr("Correction matrix (*.csv *.txt *.mat);;All files (*)"));
    if (path.isEmpty())
        return;
    const CorrectionFile f = loadCorrectionMatrix(path, m_response.isNull() ? kBands : m_response.bands());
    if (!f.ok()) {
        m_matrixLabel->setText(f.error);
        return;
    }
    m_settings.matrixPath = path;
    m_settings.loadedMatrix = f.correction;
    m_matrixLabel->setText(QStringLiteral("%1 (%2 × %3)").arg(QFileInfo(path).fileName()).arg(f.correction.outBands).arg(f.correction.inBands));
    m_fromFile->setChecked(true);
    m_enabled->setChecked(true);
    updatePreview();
}

void CorrectionDialog::updatePreview()
{
    m_fwhm->setEnabled(m_compute->isChecked() && !m_response.isNull());
    m_reg->setEnabled(m_compute->isChecked() && !m_response.isNull());
    const SpectralCorrection c = correction();
    if (!c.isValid())
        m_residual->setText(m_fromFile->isChecked() ? tr("Load a matrix file.") : tr("Load a spectral response first."));
    else if (m_fromFile->isChecked())
        m_residual->setText(tr("Matrix from %1: %2 → %3 bands, noise × %4.").arg(QFileInfo(m_settings.matrixPath).fileName())
                                .arg(c.inBands).arg(c.outBands).arg(noiseGain(c), 0, 'f', 1));
    else
        m_residual->setText(tr("Match to the target bands: %1 % residual; noise × %2. The narrower the bands, the more "
                               "noise: raise the regularisation (or the width) if the corrected image is noisy, or "
                               "denoise after the correction (MNF, guided filter).")
                                .arg(100.0 * c.residual, 0, 'f', 1).arg(noiseGain(c), 0, 'f', 1));

    // Plot: the measured responses, or C · A.
    QList<SpectrumPlot::Series> series;
    SpectralResponse shown = m_response;
    if (m_plotWhat->currentIndex() == 1 && c.isValid() && !m_response.isNull())
        shown = correctedResponse(m_response, c);
    for (int b = 0; b < shown.bands(); ++b)
        series << SpectrumPlot::Series{tr("Channel %1").arg(b + 1), classColor(b), shown.qe[b], {}, false};
    m_plot->setWavelengths(shown.lambda);
    m_plot->setSeries(series);
}

} // namespace iat
