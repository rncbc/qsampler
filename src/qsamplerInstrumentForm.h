#ifndef QSAMPLER_INSTRUMENT_FORM_H
#define QSAMPLER_INSTRUMENT_FORM_H

#include "ui_qsamplerInstrumentForm.h"

#include "qsamplerInstrument.h"

namespace QSampler {

class InstrumentForm : public QDialog {
Q_OBJECT
public:
    InstrumentForm(QWidget* parent = 0);
   ~InstrumentForm();
    void setup(qsamplerInstrument* pInstrument);

public slots:
    void nameChanged(const QString& sName);
    void openInstrumentFile();
    void updateInstrumentName();
    void instrumentNrChanged();
    void accept();
    void reject();
    void changed();
    void stabilizeForm();

protected:
    qsamplerInstrument* m_pInstrument;
    int m_iDirtySetup;
    int m_iDirtyCount;
    int m_iDirtyName;

private:
    Ui::qsamplerInstrumentForm ui;
};

} // namespace QSampler

#endif // QSAMPLER_INSTRUMENT_FORM_H
