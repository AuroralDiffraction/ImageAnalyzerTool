#pragma once

#include "core/Isp.h"

#include <QDialog>
#include <QVector>

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QToolButton;

namespace iat {

class StageNode;

// The image processing pipeline of one panel, shown as a chain of nodes:
//
//   Input ─► Black level ─► Defect pixels ─► … ─► Tone mapping ─► … ─► Output
//
// Every node is one stage of isp::Pipeline. The switch on a node turns the
// stage on or bypasses it (the wire is then drawn around the node); clicking
// a node shows its settings underneath: the algorithm and its parameters.
// Every change is reported at once with pipelineChanged(), so the panel can
// update its image while the dialog stays open.
//
// The dialog is built from the tables in core/Isp.h, so it needs no change
// when a stage gets a new algorithm or parameter.
class PipelineDialog : public QDialog {
    Q_OBJECT
public:
    explicit PipelineDialog(QWidget* parent = nullptr);

    const isp::Pipeline& pipeline() const { return m_pipeline; }
    // Shows these settings, without emitting pipelineChanged.
    void setPipeline(const isp::Pipeline& pipeline);

    // "Show original": the panel shows the unprocessed image while the
    // settings are kept, for a quick before / after comparison.
    bool showOriginal() const;
    void setShowOriginal(bool on);  // emits showOriginalChanged if it changes

    // "A/B": a divider across the panel's image, the unprocessed image to
    // its left and the processed one to its right.
    bool abCompare() const;
    void setAbCompare(bool on);  // emits abCompareChanged if it changes

    // What the last run did (isp::Result::notes) and how long it took.
    void setResult(const QStringList& notes, double milliseconds);
    void setBusy(bool busy);

    isp::StageId selectedStage() const { return m_selected; }
    void selectStage(isp::StageId id);

    // The same as the user's actions; each emits pipelineChanged.
    void setStageEnabled(isp::StageId id, bool on);
    void setStageAlgorithm(isp::StageId id, const char* algorithmId);
    void setStageValue(isp::StageId id, const char* paramId, double value);
    void applyPreset(const char* presetId);
    void resetStage(isp::StageId id);
    void resetAll();

signals:
    void pipelineChanged(const iat::isp::Pipeline& pipeline);
    void showOriginalChanged(bool on);
    void abCompareChanged(bool on);

private:
    void rebuildEditor();
    void updateNodes();
    void updateStatus();
    void changed();

    isp::Pipeline m_pipeline;
    isp::StageId m_selected = isp::StageId::ToneMap;
    QStringList m_notes;
    double m_milliseconds = -1.0;
    bool m_busy = false;

    QVector<StageNode*> m_nodes;
    QScrollArea* m_strip = nullptr;
    QWidget* m_editor = nullptr;
    QLabel* m_stageTitle = nullptr;
    QLabel* m_stageDescription = nullptr;
    QCheckBox* m_enableCheck = nullptr;
    QLabel* m_algorithmLabel = nullptr;
    QComboBox* m_algorithmCombo = nullptr;
    QLabel* m_algorithmDescription = nullptr;
    QWidget* m_paramBox = nullptr;
    QGridLayout* m_paramGrid = nullptr;
    QLabel* m_note = nullptr;
    QLabel* m_status = nullptr;
    QToolButton* m_presetButton = nullptr;
    QPushButton* m_resetStageButton = nullptr;
    QCheckBox* m_originalCheck = nullptr;
    QCheckBox* m_abCheck = nullptr;
};

} // namespace iat
