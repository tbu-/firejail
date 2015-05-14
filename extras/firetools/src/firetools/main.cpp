#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>

#include "firetools.h"
#include "mainwindow.h"
#include "../common/utils.h"

int arg_debug = 0;

// desktop file content for autostart
static const char *deskfile_minimize = 
"[Desktop Entry]\n"
"Name=Firejail Tools\n"
"Comment=Firejail user interface\n"
"Exec=firetools --minimize\n"
"Icon=firetools\n"
"Terminal=false\n"
"Type=Application\n"
"Categories=Qt;System;Security;\n";


static void usage() {
	printf("Firetools - graphic user interface for Firejail security platform\n\n");
	printf("Usage: firetools [options]\n\n");
	printf("Options:\n");
	printf("\t--autostart - configure Firetools to run automatically in system tray\n");
	printf("\t\twhen X11 session is started\n\n");
	printf("\t--debug - debug mode\n\n");
	printf("\t--help - this help screen\n\n");
	printf("\t--minimize - start the program minimized in system tray\n\n");
}

int main(int argc, char *argv[]) {
	int arg_minimize = 0;

	// parse arguments
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--debug") == 0)
			arg_debug = 1;
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-?") == 0) {
			usage();
			return 0;
		}
		else if (strcmp(argv[i], "--autostart") == 0) {
			// build directory name
			char *home = get_home_directory();
			if (!home) {
				fprintf(stderr, "Error: cannot find user home directory");
				return 1;
			}
			
			char *autofile;
			if (asprintf(&autofile, "%s/.config/autostart/firetools.desktop", home) == -1)
				errExit("asprintf");
				
			// write the file
			FILE *fp = fopen(autofile, "w");
			if (!fp) {
				fprintf(stderr, "Error: cannot open %s\n", autofile);
				return 1;
			}
			fprintf(fp, "%s\n", deskfile_minimize);
			fclose(fp);
			printf("Firetools autostart configured\n");
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
	
	// configure system tray
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
	FILE *rv = NULL;
	if (!arg_debug) {
		rv = freopen( "/dev/null", "w", stderr );
		(void) rv;
	}
	
	// start application
	int tmp = app.exec();
	(void) tmp;
	
	if (rv)
		fclose(rv);
}

