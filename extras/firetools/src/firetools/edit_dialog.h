#ifndef EDIT_DIALOG_H
#define EDIT_DIALOG_H
#include <QWidget>
#include <QDialog>
#include <QLineEdit>

class EditDialog: public QDialog {
Q_OBJECT

public:
	EditDialog(QString name, QString desc, QString cmd);
	
	
	QString getName() {
		return name_->text();
	}
	
	QString getDesc() {
		return desc_->text();
	}
	
	QString getCommand() {
		return cmd_->text();
	}
	
private:
	QLineEdit *name_;
	QLineEdit *desc_;
	QLineEdit *cmd_;
};


#endif