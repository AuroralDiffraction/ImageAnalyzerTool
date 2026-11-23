#pragma once

#include <QColor>
#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>

namespace iat {

// Line plot of spectra: one curve per series (mean, with an optional ± std
// band), against band number or wavelength. Painted with the palette, so it
// follows the light / dark theme (no Qt Charts, which is GPL-only).
class SpectrumPlot : public QWidget {
    Q_OBJECT
public:
    struct Series {
        QString name;
        QColor color;
        QVector<double> values;
        QVector<double> std;  // empty: no band
        bool dashed = false;
    };

    explicit SpectrumPlot(QWidget* parent = nullptr);

    void setSeries(const QList<Series>& series);
    const QList<Series>& series() const { return m_series; }
    // Band centres in nm; the x axis shows wavelength when they are set and
    // wavelength display is on, band numbers otherwise.
    void setWavelengths(const QVector<double>& wavelengths);
    void setShowWavelength(bool on);
    bool showsWavelength() const { return m_showWavelength && !m_wavelengths.isEmpty(); }
    // Each curve divided by its own maximum (compares shapes).
    void setNormalized(bool on);
    void setYLabel(const QString& label);
    // Legend and point markers (on by default; off for dense curves).
    void setShowLegend(bool on);
    void setShowMarkers(bool on);
    // Text of the x axis when wavelengths are shown (default "Wavelength (nm)").
    void setPlaceholder(const QString& text);

    QSize sizeHint() const override { return {360, 260}; }
    QSize minimumSizeHint() const override { return {200, 140}; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QRectF plotRect() const;
    double xOf(int band) const;
    int bands() const;

    QList<Series> m_series;
    QVector<double> m_wavelengths;
    bool m_showWavelength = true;
    bool m_normalized = false;
    QString m_yLabel;
    int m_hoverBand = -1;
    bool m_showLegend = true;
    bool m_showMarkers = true;
    QString m_placeholder;
};

} // namespace iat
