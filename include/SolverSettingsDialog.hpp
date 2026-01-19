// include/SolverSettingsDialog.hpp

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QFormLayout>

/**
 * @brief Fenêtre de dialogue permettant de configurer les paramètres du solver.
 *
 * Cette boîte de dialogue permet à l’utilisateur de choisir un préréglage de difficulté
 * ou de définir manuellement la profondeur de recherche et le temps maximum d’exécution.
 */
class SolverSettingsDialog : public QDialog {
    Q_OBJECT

  public:
    /**
     * Le constructeur crée l’interface, initialise les champs avec les valeurs
     * fournies et configure les signaux/slots nécessaires.
     * 
     * @brief Constructeur de la boîte de dialogue de configuration.
     * @param curDepth Profondeur de recherche actuelle (pour initialisation).
     * @param curTime  Temps maximum actuel en ms (pour initialisation).
     * @param parent   Widget parent (optionnel).
     */
    SolverSettingsDialog(int curDepth, int curTime, QWidget* parent = nullptr) : QDialog(parent) {

        setWindowTitle("Solver Settings");
        auto* layout = new QVBoxLayout(this);
        auto* form   = new QFormLayout();

        auto* noteLabel = new QLabel("Note: Easier presets are faster, harder presets are slower.");
        noteLabel->setWordWrap(true);
        layout->addWidget(noteLabel);

        // Liste des préréglages
        presetCombo = new QComboBox();
        presetCombo->addItem("1) Beginner (3, 100 ms)");
        presetCombo->addItem("2) Intermediate (5, 500 ms)");
        presetCombo->addItem("3) Advanced (8, 2000 ms)");
        presetCombo->addItem("4) Expert (10, 10000 ms)");
        presetCombo->addItem("5) Custom");
        form->addRow("Preset:", presetCombo);

        // SpinBox profondeur
        depthSpin = new QSpinBox();
        depthSpin->setRange(1, 15);
        depthSpin->setValue(curDepth);

        // SpinBox temps maximum
        timeSpin  = new QSpinBox();
        timeSpin->setRange(10, 60000);
        timeSpin->setValue(curTime);

        form->addRow("Depth:",         depthSpin);
        form->addRow("Max time (ms):", timeSpin);
        layout->addLayout(form);

        // Boutons OK / Annuler
        auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        layout->addWidget(btns);

        // Connexions signaux/slots
        connect(presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SolverSettingsDialog::onPresetChanged);
        connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // Initialisation avec le préréglage courant
        onPresetChanged(presetCombo->currentIndex());

        setFixedSize(sizeHint());
    }

    /**
     * @brief Retourne la profondeur de recherche choisie.
     */
    int getDepth() const { return depthSpin->value(); }

    /**
     * @brief Retourne le temps maximum (en ms) choisi.
     */
    int getTime()  const { return timeSpin->value(); }

  private slots:

    /**
     * @brief Slot appelé lorsque l’utilisateur change de préréglage.
     * @param idx Index du préréglage sélectionné.
     *
     * Cette méthode ajuste automatiquement la profondeur et le temps
     * en fonction du préréglage choisi.  
     * Si "Custom" est sélectionné, les champs deviennent éditables.
     */
    void onPresetChanged(int idx) {
        switch (idx) {
            case 0:
                depthSpin->setValue(3);
                timeSpin->setValue(100);
                depthSpin->setEnabled(false);
                timeSpin->setEnabled(false);
                break;
            case 1:
                depthSpin->setValue(5);
                timeSpin->setValue(500);
                depthSpin->setEnabled(false);
                timeSpin->setEnabled(false);
                break;
            case 2:
                depthSpin->setValue(8);
                timeSpin->setValue(2000);
                depthSpin->setEnabled(false);
                timeSpin->setEnabled(false);
                break;
            case 3:
                depthSpin->setValue(10);
                timeSpin->setValue(10000);
                depthSpin->setEnabled(false);
                timeSpin->setEnabled(false);
                break;
            case 4:
                depthSpin->setEnabled(true);
                timeSpin->setEnabled(true);
                break;
        }
    }

  private:
    QComboBox* presetCombo;
    QSpinBox*  depthSpin;
    QSpinBox*  timeSpin;
};
