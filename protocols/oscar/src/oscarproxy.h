/****************************************************************************
**
** qutIM - instant messenger
**
** Copyright © 2011 Alexey Prokhin <alexey.prokhin@yandex.ru>
**
*****************************************************************************
**
** $QUTIM_BEGIN_LICENSE$
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see http://www.gnu.org/licenses/.
** $QUTIM_END_LICENSE$
**
****************************************************************************/

#ifndef OSCARPROXY_H
#define OSCARPROXY_H

#include "icq_global.h"
#include <qutim/networkproxy.h>

namespace qutim_sdk_0_3 {
namespace oscar {

class OscarProxyManager : public QObject, public NetworkProxyManager
{
	Q_OBJECT
	Q_CLASSINFO("DependsOn", "qutim_sdk_0_3::oscar::IcqProtocol")
	Q_INTERFACES(qutim_sdk_0_3::NetworkProxyManager)
public:
	OscarProxyManager();
	QList<NetworkProxyInfo*> proxies();
	void setProxy(Account *account, NetworkProxyInfo *proxy, const DataItem &settings);
};

} } // namespace qutim_sdk_0_3::oscar

#endif // OSCARPROXY_H

