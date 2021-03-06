/*
 * This file is part of the sigrok project.
 *
 * Copyright (C) 2011 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

extern "C" {
#include <sigrokdecode.h>
#include <glib.h>
}

#include <QLabel>
#include "decodersform.h"
#include "ui_decodersform.h"

DecodersForm::DecodersForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DecodersForm)
{
	int i;
	GSList *ll;
	struct srd_decoder *dec;
	QWidget *pages[MAX_NUM_DECODERS];

	ui->setupUi(this);

	for (ll = srd_list_decoders(), i = 0; ll; ll = ll->next, ++i) {
		dec = (struct srd_decoder *)ll->data;

		/* Add the decoder to the list. */
		new QListWidgetItem(QString(dec->name), ui->listWidget);

		/* Add a page for the decoder details. */
		pages[i] = new QWidget;

		/* Add some decoder data to that page. */
		QVBoxLayout *l = new QVBoxLayout;
		l->addWidget(new QLabel("ID: " + QString(dec->id)));
		l->addWidget(new QLabel("Name: " + QString(dec->name)));
		l->addWidget(new QLabel("Long name: " + QString(dec->longname)));
		l->addWidget(new QLabel("Desc: " + QString(dec->desc)));
		l->addWidget(new QLabel("Long desc: " + QString(dec->longdesc)));
		l->addWidget(new QLabel("Author: " + QString(dec->author)));
		l->addWidget(new QLabel("Email: " + QString(dec->email)));
		l->addWidget(new QLabel("License: " + QString(dec->license)));
		l->insertStretch(-1);

		pages[i]->setLayout(l);

		/* Add the decoder's page to the stackedWidget. */
		ui->stackedWidget->addWidget(pages[i]);
	}
}

DecodersForm::~DecodersForm()
{
	delete ui;
}

void DecodersForm::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void DecodersForm::on_closeButton_clicked()
{
	close();
}

void DecodersForm::on_listWidget_currentItemChanged(QListWidgetItem *current,
						    QListWidgetItem *previous)
{
	if (!current)
		current = previous;

	ui->stackedWidget->setCurrentIndex(ui->listWidget->row(current));
}
