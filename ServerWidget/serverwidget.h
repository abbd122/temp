#pragma once

#include <QtWidgets/QWidget>
#include "ui_serverwidget.h"

class ServerWidget : public QWidget {
	Q_OBJECT

public:
	ServerWidget(QWidget* parent = nullptr);

	~ServerWidget();

private:
	Ui::ServerWidgetClass ui;
};
