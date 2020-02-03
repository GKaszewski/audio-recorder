#include "app.h"

#include <QFileDialog>
#include <QStandardPaths>

#include <QAudioProbe>
#include <QAudioRecorder>
#include <QDir>
#include <QFileDialog>
#include <QMediaRecorder>
#include <QDateTime>

#include "json.hpp"


App::App(QWidget *parent) : QMainWindow(parent), outputLocationSet(false), appSettings(parent, "settings.json") {
	ui.setupUi(this);
	setWindowTitle("Audio Recorder by Nitros");
	
	audioRecorder = new QAudioRecorder;
	//setup ui elements
	setupUIElements();
	setupAudioRecorder();

	setPath(QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" + QDateTime::currentDateTime().toString("hh-mm-ss-z") + ".wav");

	setShortcut();
	qDebug() << "Is registered: " << shortcut->isRegistered();

	QObject::connect(shortcut, &QHotkey::activated, qApp, [&]() {
		record();
	});

	if (appSettings.isSuccessful) {
		qDebug() << "loading data from json!";
		shortcutEdit->setKeySequence(appSettings.getShortcut());
		setShortcut();
		audioOutputPath = appSettings.getPath() + QDateTime::currentDateTime().toString("hh-mm-ss-z") + ".wav";
		outputFileName->setText(audioOutputPath);
		audioRecorder->setOutputLocation(QUrl::fromLocalFile(audioOutputPath));
		outputLocationSet = true;
		codecsBox->setCurrentIndex(codecsBox->findData(QVariant(appSettings.getCodec())));
		devicesBox->setCurrentIndex(appSettings.getDevice());
		ui.fileContainerBox->setCurrentIndex(ui.fileContainerBox->findData(QVariant(appSettings.getContainer())));
	}
}

App::~App() {
	delete audioRecorder;
}

static QVariant boxValue(const QComboBox* box) {
	int idx = box->currentIndex();
	if (idx == -1)
		return QVariant();

	return box->itemData(idx);
}

void App::setAudioOutputFile() {
	auto path = QFileDialog::getSaveFileName(this, tr("Save audio"), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), tr("Audio files (*.wav)"));
	setPath(path);
}

void App::pathTextBoxChanged(const QString& text) {
	setPath(text);
}

void App::setCodec(const QString& text) {
	auto settings = audioRecorder->audioSettings();
	settings.setCodec(text);
	audioRecorder->setAudioSettings(settings);
}

void App::setShortcut() {
	shortcut = new QHotkey(shortcutEdit->keySequence(), true, this);
	qDebug() << "Is registered: " << shortcut->isRegistered();
	QObject::connect(shortcut, &QHotkey::activated, qApp, [&]() {
		record();
	});
}

void App::record() {
	if (audioRecorder->state() == QMediaRecorder::StoppedState && outputLocationSet) {
		audioRecorder->setAudioInput(boxValue(devicesBox).toString());

		QAudioEncoderSettings settings;
		settings.setCodec(boxValue(codecsBox).toString());
		settings.setQuality(QMultimedia::HighQuality);
		QString container = boxValue(ui.fileContainerBox).toString();

		audioRecorder->setEncodingSettings(settings, QVideoEncoderSettings(), container);
		audioRecorder->record();
		qDebug() << "I started recording.. \n";
	}
	else {
		audioRecorder->stop();
		auto oldPath = audioOutputPath.toStdString();
		auto path = QString::fromStdString(oldPath.erase(oldPath.length() - 4));
		setPath(path + "_" + QDateTime::currentDateTime().toString("hh-mm-ss-z") + ".wav");
	}
}

void App::updateDuration(qint64 duration) {
	recordingDurationLabel->setText(tr("%1 sec").arg(duration / 1000));
	qDebug() << "Duration: " << duration/1000 << "\n";
}

void App::setDevice(const QString& text) {
	selectedDevice = text;
}

void App::setupUIElements() {
	shortcutEdit = ui.shortcutBox;
	recordButton = ui.recordButton;
	outputButton = ui.openFileDialogButton;
	outputFileName = ui.filenameTextBox;
	codecsBox = ui.codecComboBox;
	devicesBox = ui.inputDevicesComboBox;
	recordingDurationLabel = ui.recordingDuration;

	QStringList devices = audioRecorder->audioInputs();
	selectedDevice = audioRecorder->defaultAudioInput();

	//audio devices
	devicesBox->addItem(tr("Default"), QVariant(QString()));
	foreach(const QString & device, audioRecorder->audioInputs()) {
		devicesBox->addItem(device, QVariant(device));
	}

	//audio codecs
	codecsBox->addItem(tr("Default"), QVariant(QString()));
	foreach(const QString & codecName, audioRecorder->supportedAudioCodecs()) {
		codecsBox->addItem(codecName, QVariant(codecName));
	}

	//containers
	ui.fileContainerBox->addItem(tr("Default"), QVariant(QString()));
	foreach(const QString& containerName, audioRecorder->supportedContainers()) {
		ui.fileContainerBox->addItem(containerName, QVariant(containerName));
	}

	connect(outputButton, SIGNAL(clicked()), this, SLOT(setAudioOutputFile()));
	connect(recordButton, SIGNAL(clicked()), this, SLOT(record()));
	connect(shortcutEdit, SIGNAL(editingFinished()), this, SLOT(setShortcut()));
	connect(outputFileName, SIGNAL(textChanged(QString)), this, SLOT(pathTextBoxChanged(QString)));
	connect(devicesBox, SIGNAL(currentTextChanged(QString)), this, SLOT(setDevice(QString)));
	connect(codecsBox, SIGNAL(currentTextChanged(QString)), this, SLOT(setCodec(QString)));
}

void App::setupAudioRecorder() {
	QAudioEncoderSettings settings;
	settings.setCodec(codecsBox->currentText());
	settings.setQuality(QMultimedia::HighQuality);

	audioRecorder->setAudioInput(selectedDevice);
	audioRecorder->setAudioSettings(settings);

	connect(audioRecorder, SIGNAL(durationChanged(qint64)), this, SLOT(updateDuration(qint64)));
	connect(audioRecorder, SIGNAL(statusChanged(QMediaRecorder::Status)), this, SLOT(updateStatus(QMediaRecorder::Status)));
	connect(audioRecorder, SIGNAL(stateChanged(QMediaRecorder::State)), this, SLOT(onStateChanged(QMediaRecorder::State)));
	connect(audioRecorder, SIGNAL(error(QMediaRecorder::Error)), this, SLOT(displayErrorMessage()));
}

void App::closeEvent(QCloseEvent* event) {
	qDebug() << "I'm saving...\n";
	appSettings.setShortcut(shortcutEdit->keySequence());
	auto rawPath = audioOutputPath.toStdString();
	auto path = rawPath.erase(rawPath.length() - 4);
	appSettings.setPath(QString::fromStdString(path));
	appSettings.setCodec(boxValue(codecsBox).toString());
	appSettings.setContainer(boxValue(ui.fileContainerBox).toString());
	appSettings.setDevice(devicesBox->currentIndex());

	appSettings.saveToJSON("settings.json");
}

void App::setPath(const QString& path) {
	audioOutputPath = path;
	outputFileName->setText(path);
	audioRecorder->setOutputLocation(QUrl::fromLocalFile(audioOutputPath));
	outputLocationSet = true;
}

void App::onStateChanged(QMediaRecorder::State state) {
	switch (state) {
	case QMediaRecorder::RecordingState:
		recordButton->setText(tr("Stop"));
		qDebug() << "I am recording.. \n";
		break;
	case QMediaRecorder::StoppedState:
		recordButton->setText(tr("Record"));
		break;
	}
}

void App::updateStatus(QMediaRecorder::Status status) {
	QString statusMessage;

	switch (status) {
	case QMediaRecorder::RecordingStatus:
		statusMessage = tr("Recording to %1").arg(audioRecorder->actualLocation().toString());
		break;
	case QMediaRecorder::PausedStatus:
		statusMessage = tr("Paused");
		break;
	case QMediaRecorder::UnloadedStatus:
	case QMediaRecorder::LoadedStatus:
		statusMessage = tr("Stopped");
	default:
		break;
	}

	if (audioRecorder->error() == QMediaRecorder::NoError)
		ui.statusBar->showMessage(statusMessage);
}

void App::displayErrorMessage() {
	recordingDurationLabel->setText(audioRecorder->errorString());
}