#ifndef QSAMPLER_OPTIONS_FORM_H
#define QSAMPLER_OPTIONS_FORM_H

#include "ui_qsamplerOptionsForm.h"

#include "qsamplerOptions.h"

namespace QSampler {

class OptionsForm : public QDialog {
Q_OBJECT
public:
    OptionsForm(QWidget* parent = 0);
   ~OptionsForm();
    void setup(qsamplerOptions* pOptions);

public slots:
    void accept();
    void reject();
    void optionsChanged();
    void stabilizeForm();
    void chooseDisplayFont();
    void chooseMessagesFont();
    void toggleDisplayEffect(bool bOn);

private:
    Ui::qsamplerOptionsForm ui;

    qsamplerOptions* m_pOptions;
    int m_iDirtySetup;
    int m_iDirtyCount;
};

} // namespace QSampler

#endif // QSAMPLER_OPTIONS_FORM_H
