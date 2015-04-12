#include <QtGui>
#include <QUrl>
#include <assert.h>
#include <sys/utsname.h>
#include "edit_dialog.h"
#include "db.h"
#include "graph.h"
#include "../common/utils.h"
#include "../../firetools_config.h"


EditDialog::EditDialog(QString name, QString desc, QString cmd): QDialog() {
	QLabel *lname = new QLabel;
	lname->setText(tr("Name"));
	QLabel *egname = new QLabel;
	egname->setText(tr("(firefox"));
	name_ = new QLineEdit;
	name_->setText(name);

	QLabel *ldesc = new QLabel;
	ldesc->setText(tr("Description"));
	QLabel *egdesc = new QLabel;
	egdesc->setText(tr("(Mozilla Firefox)"));
	desc_ = new QLineEdit;
	desc_->setText(desc);

 	QLabel *lcmd = new QLabel;
	lcmd->setText(tr("Command"));
	QLabel *egcmd = new QLabel;
	egcmd->setText(tr("(firejail --profile=/etc/firejail/generic.profile firefox)"));
	cmd_ = new QLineEdit;
	cmd_->setText(cmd);

    	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	QGridLayout *layout = new QGridLayout;
	layout->addItem(new QSpacerItem(30, 30), 0, 0);
	layout->addWidget(lname, 0, 1);
	layout->addWidget(name_, 0, 2);
	layout->addItem(new QSpacerItem(30, 30), 0, 3);
	layout->addWidget(egname, 1,2);
	
	layout->addWidget(ldesc, 2, 1);
	layout->addWidget(desc_, 2, 2);
	layout->addWidget(egdesc, 3, 2);
	
	layout->addWidget(lcmd, 4, 1);
	layout->addWidget(cmd_, 4, 2);
	layout->addWidget(egcmd, 5,2);
	
	layout->addItem(new QSpacerItem(30, 30), 6, 0);
	layout->addWidget(buttonBox, 7, 2);
	setLayout(layout);
//	resize(600, 500);
	setWindowTitle(tr("Firejail Edit"));
}	

