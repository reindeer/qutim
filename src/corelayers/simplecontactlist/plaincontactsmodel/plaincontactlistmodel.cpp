#include "plaincontactlistmodel.h"
#include "abstractcontactmodel_p.h"
#include <qutim/metacontact.h>
#include <qutim/metacontactmanager.h>
#include <qutim/mimeobjectdata.h>
#include <qutim/protocol.h>
#include <QMimeData>
#include <QMessageBox>
#include <QTimer>

namespace Core
{
namespace SimpleContactList
{

class PlainModelPrivate : public AbstractContactModelPrivate
{
public:
	QList<ContactItem*> contacts;
	QMap<Contact *, ContactItem*> contactHash;
};

inline QList<ContactItem*> &ContactItem::siblings(AbstractContactModel *m)
{
	return reinterpret_cast<PlainModel*>(m)->d_func()->contacts;
}

inline bool ContactItem::isInSelectedTag(QSet<QString> &selectedTags)
{
	return !selectedTags.intersect(tags).isEmpty();
}

PlainModel::PlainModel(QObject *parent) : AbstractContactModel(new PlainModelPrivate, parent)
{
	QTimer::singleShot(0, this, SLOT(init()));
}

PlainModel::~PlainModel()
{
}

QModelIndex PlainModel::index(int row, int, const QModelIndex &parent) const
{
	Q_D(const PlainModel);
	if (getItemType(parent) == ContactType)
		return QModelIndex();

	if(row < 0 || row >= d->contacts.size())
		return QModelIndex();
	return createIndex(row, 0, d->contacts.at(row));
}

QModelIndex PlainModel::parent(const QModelIndex &child) const
{
	Q_UNUSED(child);
	return QModelIndex();
}

int PlainModel::rowCount(const QModelIndex &parent) const
{
	Q_D(const PlainModel);
	if (getItemType(parent) == ContactType)
		return false;
	return d->contacts.size();
}

bool PlainModel::hasChildren(const QModelIndex &parent) const
{
	Q_D(const PlainModel);
	if (getItemType(parent) == ContactType)
		return false;
	return !d->contacts.isEmpty();
}

QVariant PlainModel::data(const QModelIndex &index, int role) const
{
	if (getItemType(index) == ContactType)
		return contactData<ContactItem>(index, role);
	return QVariant();
}

void PlainModel::addContact(Contact *contact)
{
	Q_D(PlainModel);
	//TODO implement more powerfull logic
	//			if (!contact->isInList())
	//				return;

	if (d->contactHash.contains(contact))
		return;

	MetaContact *meta = qobject_cast<MetaContact*>(contact);

	if (!meta) {
		meta = static_cast<MetaContact*>(contact->metaContact());
		if (meta && d->contactHash.contains(meta))
			return;
		else if (meta)
			contact = meta;
	}

	if (meta) {
		meta->installEventFilter(this);
		foreach (ChatUnit *unit, meta->lowerUnits()) {
			Contact *subContact = qobject_cast<Contact*>(unit);
			if (subContact && d->contactHash.contains(subContact))
				removeContact(subContact);
		}
	}

	connect(contact, SIGNAL(destroyed(QObject*)),
			SLOT(contactDeleted(QObject*)));
	connect(contact, SIGNAL(statusChanged(qutim_sdk_0_3::Status,qutim_sdk_0_3::Status)),
			SLOT(contactStatusChanged(qutim_sdk_0_3::Status)));
	connect(contact, SIGNAL(nameChanged(QString,QString)),
			SLOT(contactNameChanged(QString)));
	connect(contact, SIGNAL(tagsChanged(QStringList,QStringList)),
			SLOT(contactTagsChanged(QStringList)));
	connect(contact, SIGNAL(inListChanged(bool)),
			SLOT(onContactInListChanged(bool)));

	QStringList tags = contact->tags();
	if(tags.isEmpty())
		tags << tr("Without tags");

	ContactItem *item = new ContactItem;
	item->contact = contact;
	item->tags = QSet<QString>::fromList(tags);
	item->status = contact->status();
	d->contactHash.insert(contact, item);
	changeContactVisibility(item, isVisible(item));
}

bool PlainModel::containsContact(Contact *contact) const
{
	return d_func()->contactHash.contains(contact);
}

bool PlainModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole && getItemType(index) == ContactType) {
		ContactItem *item = reinterpret_cast<ContactItem *>(index.internalPointer());
		item->contact->setName(value.toString());
		return true;
	}
	return false;
}

QStringList PlainModel::mimeTypes() const
{
	QStringList types;
	types << QUTIM_MIME_CONTACT_INTERNAL;
	types << MimeObjectData::objectMimeType();
	return types;
}

QMimeData *PlainModel::mimeData(const QModelIndexList &indexes) const
{
	MimeObjectData *mimeData = new MimeObjectData();
	QModelIndex index = indexes.value(0);
	if (getItemType(index) != ContactType)
		return mimeData;

	ContactItem *item = reinterpret_cast<ContactItem*>(index.internalPointer());
	mimeData->setText(item->contact->id());
	mimeData->setObject(item->contact);
	setEncodedData(mimeData, QUTIM_MIME_CONTACT_INTERNAL, index);
	return mimeData;
}

bool PlainModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
							  int row, int column, const QModelIndex &parent)
{
	if (getItemType(parent) != ContactType)
		return false;
	return AbstractContactModel::dropMimeData(data, action, row, column, parent);
}

void PlainModel::removeFromContactList(Contact *contact, bool deleted)
{
	Q_D(PlainModel);
	Q_UNUSED(deleted);
	ContactItem *item = d->contactHash.take(contact);
	if (!item)
		return;
	changeContactVisibility(item, false);
	d->unreadContacts.remove(contact);
	delete item;
}

void PlainModel::contactDeleted(QObject *obj)
{
	Contact *contact = reinterpret_cast<Contact *>(obj);
	removeFromContactList(contact,true);
}

void PlainModel::removeContact(Contact *contact)
{
	Q_D(PlainModel);
	Q_ASSERT(contact);
	if (MetaContact *meta = qobject_cast<MetaContact*>(contact)) {
		contact->removeEventFilter(this);
		foreach (ChatUnit *unit, meta->lowerUnits()) {
			Contact *subContact = qobject_cast<Contact*>(unit);
			if (subContact && !d->contactHash.contains(subContact))
				addContact(subContact);
		}
	}
	contact->disconnect(this);
	removeFromContactList(contact,false);
}

void PlainModel::contactStatusChanged(const Status &status)
{
	Q_D(PlainModel);
	ContactItem *item = d->contactHash.value(reinterpret_cast<Contact *>(sender()));
	if (!item)
		return;
	item->status = status;
	if (!changeContactVisibility(item, isVisible(item)))
		updateContact(item, true);
}

void PlainModel::contactNameChanged(const QString &name)
{
	Q_D(PlainModel);
	Q_UNUSED(name);
	ContactItem *item = d->contactHash.value(reinterpret_cast<Contact *>(sender()));
	if (!item || !isVisible(item))
		return;
	updateContact(item, true);
}

void PlainModel::contactTagsChanged(const QStringList &tags)
{
	Q_D(PlainModel);
	ContactItem *item = d->contactHash.value(reinterpret_cast<Contact *>(sender()));
	if (!item)
		return;
	item->tags = QSet<QString>::fromList(tags);
	if (!isVisible(item))
		return;
	updateContact(item, false);
}

void PlainModel::onContactInListChanged(bool)
{
	//Contact *contact = qobject_cast<Contact*>(sender());
	//p->contacts.value(contact)->
	//TODO
}

QStringList PlainModel::tags() const
{
	Q_D(const PlainModel);
	QSet<QString> all_tags;
	foreach (ContactItem *item, d->contactHash)
		all_tags += item->tags;
	return all_tags.toList();
}

void PlainModel::init()
{
	foreach(Protocol *proto, Protocol::all()) {
		connect(proto, SIGNAL(accountCreated(qutim_sdk_0_3::Account*)), this, SLOT(onAccountCreated(qutim_sdk_0_3::Account*)));
		foreach(Account *account, proto->accounts())
			onAccountCreated(account);
	}
}

void PlainModel::filterAllList()
{
	Q_D(PlainModel);
	foreach (ContactItem *item, d->contactHash)
		changeContactVisibility(item, isVisible(item));
}

void PlainModel::updateContactData(Contact *contact)
{
	Q_D(PlainModel);
	ContactItem *item = d->contactHash.value(contact);
	Q_ASSERT(item);
	QModelIndex index = createIndex(d->contacts.indexOf(item), 0, item);
	emit dataChanged(index, index);
}

bool PlainModel::changeContactVisibility(ContactItem *item, bool visibility)
{
	Q_D(PlainModel);
	int row = d->contacts.indexOf(item);
	if ((row != -1) == visibility)
		return false;

	if (visibility) {
		QList<ContactItem*>::const_iterator it =
				qLowerBound(d->contacts.constBegin(), d->contacts.constEnd(), item, contactLessThan<ContactItem>);
		int row = it - d->contacts.constBegin();
		beginInsertRows(QModelIndex(), row, row);
		d->contacts.insert(row, item);
		endInsertRows();
	} else {
		Q_ASSERT(row != -1);
		beginRemoveRows(QModelIndex(), row, row);
		d->contacts.removeAt(row);
		endRemoveRows();
	}
	return true;
}

void PlainModel::processEvent(ChangeEvent *ev)
{
	Q_UNUSED(ev);
	if (ev->type == ChangeEvent::MergeContacts) { // MetaContacts
		ContactItem *item = reinterpret_cast<ContactItem*>(ev->child);
		ContactItem *parent = reinterpret_cast<ContactItem*>(ev->parent);
		showContactMergeDialog(parent, item);
	}
}

bool PlainModel::eventFilter(QObject *obj, QEvent *ev)
{
	if (ev->type() == MetaContactChangeEvent::eventType()) {
		MetaContactChangeEvent *metaEvent = static_cast<MetaContactChangeEvent*>(ev);
		if (metaEvent->oldMetaContact() && !metaEvent->newMetaContact())
			addContact(metaEvent->contact());
		else if (!metaEvent->oldMetaContact() && metaEvent->newMetaContact())
			removeContact(metaEvent->contact());
		return false;
	}
	return QAbstractItemModel::eventFilter(obj, ev);
}

void PlainModel::onAccountCreated(qutim_sdk_0_3::Account *account)
{
	foreach (Contact *contact, account->findChildren<Contact*>())
		addContact(contact);
	connect(account, SIGNAL(contactCreated(qutim_sdk_0_3::Contact*)),
			this, SLOT(addContact(qutim_sdk_0_3::Contact*)));
}

}
}


