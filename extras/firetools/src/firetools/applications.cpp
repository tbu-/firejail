#include <stdio.h>
#include "firetools.h"
#include "applications.h"
#include "../common/utils.h"
#include <QDirIterator>
#include <QPainter>
QList<Application> applist;

Application::Application(const char *name, const char *exec, const char *icon):
	name_(name), exec_(exec), icon_(icon) {
	
	exec_ += " &";
	app_icon_ = loadIcon(icon_);
};

/*
From: http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html

Icons and themes are looked for in a set of directories. By default, apps should look 
in $HOME/.icons (for backwards compatibility), in $XDG_DATA_DIRS/icons and in /
usr/share/pixmaps (in that order). Applications may further add their own icon 
directories to this list, and users may extend or change the list (in application/desktop 
specific ways).In each of these directories themes are stored as subdirectories. 
A theme can be spread across several base directories by having subdirectories of 
the same name. This way users can extend and override system themes.

In order to have a place for third party applications to install their icons there 
should always exist a theme called "hicolor" [1]. The data for the hicolor theme is 
available for download at: http://www.freedesktop.org/software/icon-theme/. I
mplementations are required to look in the "hicolor" theme if an icon was not found 
in the current theme. 
*/
QIcon Application::loadIcon(QString name) {
	if (name.startsWith('/') || name.startsWith(":resources")) {
		if (arg_debug)
			printf("icon %s: full path\n", name.toLocal8Bit().data());
		return QIcon(name);
	}
	
	// look for the file in firejail config directory under /home/user
	QString conf = QDir::homePath() + "/.config/firejail/" + name + ".png";
	QFileInfo checkFile1(conf);
	if (checkFile1.exists() && checkFile1.isFile()) {
		if (arg_debug)
			printf("icon %s: local config dir\n", name.toLocal8Bit().data());
		return QIcon(conf);
	}
	conf = QDir::homePath() + "/.config/firejail/" + name + ".jpg";
	QFileInfo checkFile2(conf);
	if (checkFile2.exists() && checkFile2.isFile()) {
		if (arg_debug)
			printf("icon %s: local config dir\n", name.toLocal8Bit().data());
		return QIcon(conf);
	}
	conf = QDir::homePath() + "/.config/firejail/" + name + ".svg";
	QFileInfo checkFile3(conf);
	if (checkFile3.exists() && checkFile3.isFile()) {
		if (arg_debug)
			printf("icon %s: local config dir\n", name.toLocal8Bit().data());
		return QIcon(conf);
	}
	
	
	{
		QDirIterator it("/usr/share/icons/hicolor/scalable", QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			QFileInfo fi = it.fileInfo();
			if (fi.isFile() && fi.baseName() == name) {
				if (arg_debug)
					printf("icon %s: scalable\n", name.toLocal8Bit().data());
				return QIcon(fi.canonicalFilePath());
			}
		}
	}

	{
		QDirIterator it("/usr/share/icons/hicolor/64x64", QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			QFileInfo fi = it.fileInfo();
			if (fi.isFile() && fi.baseName() == name) {
				if (arg_debug)
					printf("icon %s: 64x64\n", name.toLocal8Bit().data());
				return QIcon(fi.canonicalFilePath());
			}
		}
	}
	
	{
		QDirIterator it("/usr/share/icons/hicolor/128x128", QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			QFileInfo fi = it.fileInfo();
			if (fi.isFile() && fi.baseName() == name) {
				if (arg_debug)
					printf("icon %s: 128x128\n", name.toLocal8Bit().data());
				return QIcon(fi.canonicalFilePath());
			}
		}
	}
	
	{
		QDirIterator it("/usr/share/icons/hicolor/256x256", QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			QFileInfo fi = it.fileInfo();
			if (fi.isFile() && fi.baseName() == name) {
				if (arg_debug)
					printf("icon %s: 256x256\n", name.toLocal8Bit().data());
				return QIcon(fi.canonicalFilePath());
			}
		}
	}

	{	
		QDirIterator it("/usr/share/pixmaps", QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			QFileInfo fi = it.fileInfo();
			if (fi.isFile() && fi.baseName() == name) {
				printf("icon %s: /usr/share/pixmaps\n", name.toLocal8Bit().data());
				QIcon icon = QIcon(fi.canonicalFilePath());
#if 0 // scale				
				QSize sz = icon.actualSize(QSize(64, 64));
				if (sz.height() < 64 && sz.width() < 64) {
					QPixmap pix = icon.pixmap(sz.height(), sz.width());
					QPixmap newpix = pix.scaled(50, 50);
					return QIcon(newpix);
				}
#endif
				return icon;
				
			}
		}
	}	
	
	if (QIcon::hasThemeIcon(name)) {
		if (arg_debug)
			printf("icon %s: fromTheme\n", name.toLocal8Bit().data());
		return QIcon::fromTheme(name);
	}
	
	// create a new icon
	if (arg_debug)
		printf("icon %s: created\n", name.toLocal8Bit().data());
	QPixmap pix(64, 64);
	pix.fill(Qt::red);
	QPainter painter( &pix );
	painter.setPen(Qt::white);
	painter.setFont(QFont("Sans"));
	painter.drawText(3, 20, name);
	painter.end();
	QIcon icon(pix);
	return icon;
}


void applications_init() {
	applist.append(Application("Firejail Tools and Statistics", "firestats", ":resources/firestats.png"));

	if (which("iceweasel"))
		applist.append(Application("Debian Iceweasel", "firejail iceweasel", "iceweasel"));
	else if (which("firefox"))
		applist.append(Application("Mozilla Firefox", "firejail firefox", "firefox"));

	if (which("chromium"))
		applist.append(Application("Chromium Web Browser", "firejail chromium", "chromium"));
	else if (which("chromium-browser"))
		applist.append(Application("Chromium Web Browser", "firejail chromium-browser", "chromium-browser"));
	
	if (which("midori"))
		applist.append(Application("Midori Web Browser", "firejail midori", "midori"));

	if (which("opera"))
		applist.append(Application("Opera Web Browser", "firejail opera", "opera"));

	if (which("icedove"))
		applist.append(Application("Debian Icedove", "firejail icedove", ":resources/icedove.png"));
	else if (which("thunderbird"))
		applist.append(Application("Thunderbird", "firejail thunderbird", ":resources/icedove.png"));

	if (which("evince"))
		applist.append(Application("Evince PDF viewer", "firejail evince", "evince"));

	if (which("transmission-gtk"))
		applist.append(Application("Transmission BitTorrent Client", "firejail transmission-gtk", "transmission"));
	else if (which("transmission-qt"))
		applist.append(Application("Transmission BitTorrent Client", "firejail transmission-qt", "transmission"));

	if (which("deluge"))
		applist.append(Application("Deluge BitTorrent Client", "firejail deluge", "deluge"));

	if (which("qbittorrent"))
		applist.append(Application("qBittorrent Client", "firejail qbittorrent", "qbittorrent"));

	if (which("vlc"))
		applist.append(Application("VideoLAN Client", "firejail vlc", "vlc"));

	if (which("rhythmbox"))
		applist.append(Application("Rhythmbox", "firejail rhythmbox", "rhythmbox"));

	if (which("totem"))
		applist.append(Application("Totem", "firejail totem", "totem"));

	if (which("audacious"))
		applist.append(Application("Audacious", "firejail audacious", "audacious"));

	if (which("gnome-mplayer"))
		applist.append(Application("GNOME MPlayer", "firejail gnome-mplayer", "gnome-mplayer"));

	if (which("clementine"))
		applist.append(Application("Clementine", "firejail clementine", "application-x-clementine"));
}


int applications_get_index(QPoint pos) {
	int nelem = applist.count();
	int cols = nelem / ROWS + 1;

	if (pos.y() < (MARGIN * 2 + TOP))
		return -1;

	if (pos.x() > (MARGIN * 2) && pos.x() < (MARGIN * 2 + cols * 64)) {
		int index_y = (pos.y() - 2 * MARGIN - TOP) / 64;
		int index_x = (pos.x() - 2 * MARGIN) / 64;
		int index = index_y + index_x * ROWS;

		if (index < nelem) {
			return index;
		}
	}
	return -1;
}
