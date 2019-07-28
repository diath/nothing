#include <QApplication>

#include "mainwindow.hpp"

int main(int argc, char **argv)
{
	QApplication app{argc, argv};

	QCoreApplication::setOrganizationName("diath");
	QCoreApplication::setApplicationName("nothing");
	QCoreApplication::setApplicationVersion("0.1");

	MainWindow window{argc, argv};
	window.show();

	return app.exec();
}
