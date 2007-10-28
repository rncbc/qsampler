#ifndef QSAMPLER_INSTRUMENT_LIST_FORM_H
#define QSAMPLER_INSTRUMENT_LIST_FORM_H

#include "ui_qsamplerInstrumentListForm.h"

#include "qsamplerInstrumentList.h"

#include <QComboBox>

namespace QSampler {

class InstrumentListForm : public QMainWindow {
Q_OBJECT
public:
    MidiInstrumentsModel model;

    InstrumentListForm(QWidget* parent = 0, Qt::WindowFlags flags = 0);
   ~InstrumentListForm();

public slots:
    void refreshInstruments();
    void activateMap(int);

protected:
    QComboBox* m_pMapComboBox;
    QToolBar* InstrumentToolbar;

    void showEvent(QShowEvent* pShowEvent);
    void hideEvent(QHideEvent* pHideEvent);

private:
    Ui::qsamplerInstrumentListForm ui;
};

} // namespace QSampler

#endif // QSAMPLER_INSTRUMENT_LIST_FORM_H
