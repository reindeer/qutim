#ifndef NOTIFICATIONSSETTINGS_H
#define NOTIFICATIONSSETTINGS_H

#include <QObject>
#include <qutim/startupmodule.h>

namespace Core
{

	class NotificationsSettings : public QObject
	{
		Q_OBJECT
	public:
		explicit NotificationsSettings(QObject *parent = 0);
	};

}

#endif // NOTIFICATIONSSETTINGS_H
