#ifndef APPLICATIONS_H
#define APPLICATIONS_H
#include <QList>
#include <QString>
#include <QIcon>

#define TOP 10
#define MARGIN 5
#define AFRAMES 6	// animation frames
#define ADELAY 20		// animation delay
#define ROWS 6

struct Application {
	QString name_;
	QString description_;
	QString exec_;
	QString icon_;
	QIcon app_icon_;
	
	Application(const char *name, const char *description, const char *exec, const char *icon);
	QIcon loadIcon(QString name);
};

extern QList<Application> applist;
void applications_init();
int applications_get_index(QPoint pos);
int applications_get_position(QPoint pos);

#endif