#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QWidget>
#include <QAction>
#include <QSystemTrayIcon>

class PidThread;
class StatsDialog;

class MainWindow : public QWidget {
Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	QSize sizeHint() const;

protected:
	void mouseMoveEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	bool event(QEvent *event);

public slots:
	void trayActivated(QSystemTrayIcon::ActivationReason);
	void cycleReady();
	
private slots:
	void edit();
	void remove();
	void run();
	void help();

signals:
	void cycleReadySignal();

	

private:
    	void createTrayActions();
   	void createLocalActions();
	
private:
	QPoint dragPosition_;
	QAction *qedit_;
	QAction *qrun_;
	QAction *qhelp_;
	QAction *qdelete_;
	int active_index_;
	int animation_id_;
	int edit_index_;
	PidThread *thread_;
	StatsDialog *stats_;
	
public:	
	// tray
	QAction *minimizeAction;
	QAction *restoreAction;
	QAction *quitAction;
	
};
#endif
