#include <QtGui>
#include <QUrl>
#include <assert.h>
#include <sys/utsname.h>
#include "edit_dialog.h"
#include "db.h"
#include "graph.h"
#include "applications.h"
#include "../common/utils.h"
#include "../../firetools_config.h"



EditDialog::EditDialog(QString name, QString desc, QString cmd): QDialog() {
	// editing
	QLabel *lname = new QLabel;
	lname->setText(tr("Name"));
	name_ = new QLineEdit;
	name_->setText(name);
	if (applications_check_default(name.toLocal8Bit().data()))
		name_->setEnabled(false);

	QLabel *ldesc = new QLabel;
	ldesc->setText(tr("Description"));
	desc_ = new QLineEdit;
	desc_->setText(desc);

 	QLabel *lcmd = new QLabel;
	lcmd->setText(tr("Command"));
	cmd_ = new QLineEdit;
	cmd_->setText(cmd);

	// buttons
    	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	QPushButton *helpButton = new QPushButton("Help");
	connect(helpButton, SIGNAL(pressed()), this, SLOT(help()));

	// layout- editing
	QGridLayout *layout = new QGridLayout;
	layout->addItem(new QSpacerItem(30, 15), 0, 0);
	layout->addItem(new QSpacerItem(400, 15), 0, 2);
	layout->addItem(new QSpacerItem(30, 15), 0, 3);
	layout->addWidget(lname, 1, 1);
	layout->addWidget(name_, 1, 2);
	layout->addWidget(ldesc, 2, 1);
	layout->addWidget(desc_, 2, 2);
	layout->addWidget(lcmd, 3, 1);
	layout->addWidget(cmd_, 3, 2);
	layout->addItem(new QSpacerItem(30, 15), 4, 0);
	
	// layout - buttons
	layout->addItem(new QSpacerItem(30, 30), 5, 0);
	layout->addWidget(helpButton, 6, 1);
	layout->addWidget(buttonBox, 6, 2);
	layout->addItem(new QSpacerItem(30, 15), 7, 0);
	setLayout(layout);
//	resize(600, 500);
	setWindowTitle(tr("Edit Sandbox"));
}	

void EditDialog::help() {
	QMessageBox msgBox;
	
	QString txt;
	txt += "<br/>";
	txt += "<b>Name:</b> unique name for this launcher<br/>\n";
	txt += "<b>Description:</b> a short description of the program<br/>\n";
	txt += "<b>Command:</b> command for starting the program<br/><br/>\n";
	txt += "<b>Example</b><br/><br/>\n";
	txt += "Name: firefox<br/>\n";
	txt += "Description: Mozilla Firefox Browser<br/>\n";
	txt += "Command: firejail firefox<br>\n";
	txt += "<br/>";

	QMessageBox::about(this, tr("Editing Sandbox Entries"), txt);
}

