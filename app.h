#pragma once

#include <QtWidgets/QMainWindow>
#include <QCloseEvent>
#include <QString>
#include <QUrl>
#include <QKeySequenceEdit>
#include <QDebug>

#include <QAudioRecorder>
#include <QAudioBuffer>
#include <QMediaRecorder>

#include "ui_app.h"

#include "QHotkey/qhotkey.h"
#include "appsettings.h"

class QAudioRecorder;
class QAudioProbe;
class QAudioBuffer;

class App : public QMainWindow {
	Q_OBJECT

public:
	App(QWidget *parent = Q_NULLPTR);
	~App();

private slots:
	void setAudioOutputFile();
	void pathTextBoxChanged(const QString& text);
	void setCodec(const QString& text);
	void setShortcut();
	void record();
	void updateDuration(qint64 duration);
	void setDevice(const QString& text);
	void updateStatus(QMediaRecorder::Status);

	void onStateChanged(QMediaRecorder::State);
	void displayErrorMessage();

private:
	Ui::AppClass ui;
	QString audioOutputPath;
	QString selectedDevice;
	QHotkey* shortcut;
	QComboBox* codecsBox;
	QComboBox* devicesBox;
	QPushButton* recordButton;
	QPushButton* outputButton;
	QLineEdit* outputFileName;
	QKeySequenceEdit* shortcutEdit;
	QLabel* recordingDurationLabel;
	QAudioRecorder* audioRecorder;
	bool outputLocationSet;
	AppSettings appSettings;

	bool isRecording;

	void setupUIElements();
	void setupAudioRecorder();
	void closeEvent(QCloseEvent* event) override;
	void setPath(const QString& path);
};
