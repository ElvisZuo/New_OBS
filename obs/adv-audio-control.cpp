#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include "qt-wrappers.hpp"
#include "adv-audio-control.hpp"

#ifndef NSEC_PER_MSEC
#define NSEC_PER_MSEC 1000000
#endif

OBSAdvAudioCtrl::OBSAdvAudioCtrl(QGridLayout *layout, obs_source_t *source_)
	: source(source_)
{
	QHBoxLayout *hlayout;
	signal_handler_t *handler = obs_source_get_signal_handler(source);
	const char *sourceName = obs_source_get_name(source);
	float vol = obs_source_get_volume(source);
	uint32_t flags = obs_source_get_flags(source);
	uint32_t mixers = obs_source_get_audio_mixers(source);

	forceMonoContainer             = new QWidget();
	mixerContainer                 = new QWidget();
	panningContainer               = new QWidget();
	labelL                         = new QLabel();
	labelR                         = new QLabel();
	nameLabel                      = new QLabel();
	volume                         = new QSpinBox();
	forceMono                      = new QCheckBox();
	panning                        = new QSlider(Qt::Horizontal);
	syncOffset                     = new QSpinBox();
	mixer1                         = new QCheckBox();
	mixer2                         = new QCheckBox();
	mixer3                         = new QCheckBox();
	mixer4                         = new QCheckBox();

	volChangedSignal.Connect(handler, "volume", OBSSourceVolumeChanged,
			this);
	syncOffsetSignal.Connect(handler, "audio_sync", OBSSourceSyncChanged,
			this);
	flagsSignal.Connect(handler, "update_flags", OBSSourceFlagsChanged,
			this);
	mixersSignal.Connect(handler, "audio_mixers", OBSSourceMixersChanged,
			this);

	hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0, 0, 0, 0);
	forceMonoContainer->setLayout(hlayout);
	hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0, 0, 0, 0);
	mixerContainer->setLayout(hlayout);
	hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0, 0, 0, 0);
	panningContainer->setLayout(hlayout);
	panningContainer->setMinimumWidth(100);

	labelL->setText("L");

	labelR->setText("R");

	nameLabel->setMinimumWidth(170);
	nameLabel->setText(QT_UTF8(sourceName));
	nameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	volume->setMinimum(0);
	volume->setMaximum(2000);
	volume->setValue(int(vol * 100.0f));

	forceMono->setChecked((flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0);

	forceMonoContainer->layout()->addWidget(forceMono);
	forceMonoContainer->layout()->setAlignment(forceMono,
			Qt::AlignHCenter | Qt::AlignVCenter);

	panning->setMinimum(0);
	panning->setMaximum(100);
	panning->setTickPosition(QSlider::TicksAbove);
	panning->setEnabled(false);
	panning->setValue(50); /* XXX */

	int64_t cur_sync = obs_source_get_sync_offset(source);
	syncOffset->setMinimum(-20000);
	syncOffset->setMaximum(20000);
	syncOffset->setValue(int(cur_sync / NSEC_PER_MSEC));

	mixer1->setText("1");
	mixer1->setChecked(mixers & (1<<0));
	mixer2->setText("2");
	mixer2->setChecked(mixers & (1<<1));
	mixer3->setText("3");
	mixer3->setChecked(mixers & (1<<2));
	mixer4->setText("4");
	mixer4->setChecked(mixers & (1<<3));

	panningContainer->layout()->addWidget(labelL);
	panningContainer->layout()->addWidget(panning);
	panningContainer->layout()->addWidget(labelR);
	panningContainer->setMaximumWidth(170);

	mixerContainer->layout()->addWidget(mixer1);
	mixerContainer->layout()->addWidget(mixer2);
	mixerContainer->layout()->addWidget(mixer3);
	mixerContainer->layout()->addWidget(mixer4);

	QWidget::connect(volume, SIGNAL(valueChanged(int)),
			this, SLOT(volumeChanged(int)));
	QWidget::connect(forceMono, SIGNAL(clicked(bool)),
			this, SLOT(downmixMonoChanged(bool)));
	QWidget::connect(panning, SIGNAL(valueChanged(int)),
			this, SLOT(panningChanged(int)));
	QWidget::connect(syncOffset, SIGNAL(valueChanged(int)),
			this, SLOT(syncOffsetChanged(int)));
	QWidget::connect(mixer1, SIGNAL(clicked(bool)),
			this, SLOT(mixer1Changed(bool)));
	QWidget::connect(mixer2, SIGNAL(clicked(bool)),
			this, SLOT(mixer2Changed(bool)));
	QWidget::connect(mixer3, SIGNAL(clicked(bool)),
			this, SLOT(mixer3Changed(bool)));
	QWidget::connect(mixer4, SIGNAL(clicked(bool)),
			this, SLOT(mixer4Changed(bool)));

	int lastRow = layout->rowCount();

	layout->addWidget(nameLabel, lastRow, 0);
	layout->addWidget(volume, lastRow, 1);
	layout->addWidget(forceMonoContainer, lastRow, 2);
	layout->addWidget(panningContainer, lastRow, 3);
	layout->addWidget(syncOffset, lastRow, 4);
	layout->addWidget(mixerContainer, lastRow, 5);
	layout->layout()->setAlignment(mixerContainer,
			Qt::AlignHCenter | Qt::AlignVCenter);
}

OBSAdvAudioCtrl::~OBSAdvAudioCtrl()
{
	nameLabel->deleteLater();
	volume->deleteLater();
	forceMonoContainer->deleteLater();
	panningContainer->deleteLater();
	syncOffset->deleteLater();
	mixerContainer->deleteLater();
}

/* ------------------------------------------------------------------------- */
/* OBS source callbacks */

void OBSAdvAudioCtrl::OBSSourceFlagsChanged(void *param, calldata_t *calldata)
{
	uint32_t flags = (uint32_t)calldata_int(calldata, "flags");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl*>(param),
			"SourceFlagsChanged", Q_ARG(uint32_t, flags));

}

void OBSAdvAudioCtrl::OBSSourceVolumeChanged(void *param, calldata_t *calldata)
{
	float volume = (float)calldata_float(calldata, "volume");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl*>(param),
			"SourceVolumeChanged", Q_ARG(float, volume));
}

void OBSAdvAudioCtrl::OBSSourceSyncChanged(void *param, calldata_t *calldata)
{
	int64_t offset = calldata_int(calldata, "offset");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl*>(param),
			"SourceSyncChanged", Q_ARG(int64_t, offset));
}

void OBSAdvAudioCtrl::OBSSourceMixersChanged(void *param, calldata_t *calldata)
{
	uint32_t mixers = (uint32_t)calldata_int(calldata, "mixers");
	QMetaObject::invokeMethod(reinterpret_cast<OBSAdvAudioCtrl*>(param),
			"SourceMixersChanged", Q_ARG(uint32_t, mixers));
}

/* ------------------------------------------------------------------------- */
/* Qt event queue source callbacks */

static inline void setCheckboxState(QCheckBox *checkbox, bool checked)
{
	checkbox->blockSignals(true);
	checkbox->setChecked(checked);
	checkbox->blockSignals(false);
}

void OBSAdvAudioCtrl::SourceFlagsChanged(uint32_t flags)
{
	bool forceMonoVal = (flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0;
	setCheckboxState(forceMono, forceMonoVal);
}

void OBSAdvAudioCtrl::SourceVolumeChanged(float value)
{
	volume->blockSignals(true);
	volume->setValue(int(round(value * 100.0f)));
	volume->blockSignals(false);
}

void OBSAdvAudioCtrl::SourceSyncChanged(int64_t offset)
{
	syncOffset->setValue(offset / NSEC_PER_MSEC);
}

void OBSAdvAudioCtrl::SourceMixersChanged(uint32_t mixers)
{
	setCheckboxState(mixer1, mixers & (1<<0));
	setCheckboxState(mixer2, mixers & (1<<1));
	setCheckboxState(mixer3, mixers & (1<<2));
	setCheckboxState(mixer4, mixers & (1<<3));
}

/* ------------------------------------------------------------------------- */
/* Qt control callbacks */

void OBSAdvAudioCtrl::volumeChanged(int percentage)
{
	float val = float(percentage) / 100.0f;
	obs_source_set_volume(source, val);
}

void OBSAdvAudioCtrl::downmixMonoChanged(bool checked)
{
	uint32_t flags = obs_source_get_flags(source);
	bool forceMonoActive = (flags & OBS_SOURCE_FLAG_FORCE_MONO) != 0;

	if (forceMonoActive != checked) {
		if (checked)
			flags |= OBS_SOURCE_FLAG_FORCE_MONO;
		else
			flags &= ~OBS_SOURCE_FLAG_FORCE_MONO;

		obs_source_set_flags(source, flags);
	}
}

void OBSAdvAudioCtrl::panningChanged(int val)
{
	/* TODO */
	UNUSED_PARAMETER(val);
}

void OBSAdvAudioCtrl::syncOffsetChanged(int milliseconds)
{
	int64_t cur_val = obs_source_get_sync_offset(source);

	if (cur_val / NSEC_PER_MSEC != milliseconds)
		obs_source_set_sync_offset(source,
				int64_t(milliseconds) * NSEC_PER_MSEC);
}

static inline void setMixer(obs_source_t *source, const int mixerIdx,
		const bool checked)
{
	uint32_t mixers = obs_source_get_audio_mixers(source);
	uint32_t new_mixers = mixers;

	if (checked) new_mixers |=  (1<<mixerIdx);
	else         new_mixers &= ~(1<<mixerIdx);

	obs_source_set_audio_mixers(source, new_mixers);
}

void OBSAdvAudioCtrl::mixer1Changed(bool checked)
{
	setMixer(source, 0, checked);
}

void OBSAdvAudioCtrl::mixer2Changed(bool checked)
{
	setMixer(source, 1, checked);
}

void OBSAdvAudioCtrl::mixer3Changed(bool checked)
{
	setMixer(source, 2, checked);
}

void OBSAdvAudioCtrl::mixer4Changed(bool checked)
{
	setMixer(source, 3, checked);
}
