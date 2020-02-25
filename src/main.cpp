#include <QApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QCommandLineParser>
#include "wbt_stuff.h"
#include "mainwindow.h"


int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QGuiApplication::setApplicationDisplayName(ImageViewer::tr("Image Viewer"));
	ImageViewer imageViewer;
	imageViewer.show();
	ExpObj qt1 = ExpObj();
	QObject::connect(&qt1, &ExpObj::sig_display_image, &imageViewer, &ImageViewer::recv_image_data);
	QObject::connect(&imageViewer, &ImageViewer::sig_image_viewed, &qt1, &ExpObj::wlbt_image_processing);
	QThread *wrk_thread = new QThread();
	qt1.moveToThread(wrk_thread);
	wrk_thread->start();


	QTimer::singleShot(0, &qt1, &ExpObj::start);
	return app.exec();
}
