#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>

#include <stdio.h>
#include "firetools.h"
#include "mainwindow.h"

int arg_debug = 0;

static void usage() {
	printf("Firetools - graphic user interface for Firejail security platform\n\n");
	printf("Usage: firetools [options]\n\n");
	printf("Options:\n");
	printf("\t--debug - debug mode\n\n");
	printf("\t--help - this help screen\n\n");
	printf("\t--minimize - start the program minimized in system tray\n\n");
}

int main(int argc, char *argv[]) {
	int arg_minimize = 0;
	
	// parse arguments
	int i;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--debug") == 0)
			arg_debug = 1;
		else if (strcmp(argv[i], "--help") == 0) {
			usage();
			return 0;
		}
		else if (strcmp(argv[i], "--minimize") == 0)
			arg_minimize = 1;
		else {
			fprintf(stderr, "Error: invalid option\n");
			usage();
			return 1;
		}		
	}
	
	// initialize resources
	Q_INIT_RESOURCE(firetools);

	QApplication app(argc, argv);
	MainWindow fc;
	if (!arg_minimize)
		fc.show();
	
	// system tray
	QSystemTrayIcon icon(QIcon(":resources/firetools.png"));
	icon.show();	
	icon.setToolTip("Firetools (click to open)");
	QMenu *trayIconMenu = new QMenu(&fc);
	trayIconMenu->addAction(fc.minimizeAction);
	trayIconMenu->addAction(fc.restoreAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(fc.quitAction);
	icon.setContextMenu(trayIconMenu);
	icon.connect(&icon, SIGNAL(activated(QSystemTrayIcon: :ActivationReason)), &fc, SLOT(trayActivated(QSystemTrayIcon: :ActivationReason)));
	
	// direct all errror to /dev/null to work around this qt bug:
	//      https://bugreports.qt.io/browse/QTBUG-43270
	FILE *rv = freopen( "/dev/null", "w", stderr );
	(void) rv;
	int tmp = app.exec();
	(void) tmp;
	if (rv)
		fclose(rv);
}

