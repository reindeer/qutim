#ifndef DATAFORMSBACKEND_H
#define DATAFORMSBACKEND_H

#include <qutim/dataforms.h>
#include "abstractdatawidget.h"

class QAbstractButton;
class QDialogButtonBox;

namespace Core
{

using namespace qutim_sdk_0_3;

class DefaultDataForm : public AbstractDataForm
{
	Q_OBJECT
public:
	DefaultDataForm(const DataItem &item, StandardButtons standartButtons = NoButton,  const Buttons &buttons = Buttons());
	virtual DataItem item() const;
	virtual bool isChanged() const;
	virtual bool isComplete() const;
	virtual void clearState();
	virtual void setData(const QString &name, const QVariant &data);
	inline void addWidget(const QString &name, AbstractDataWidget *widget)
	{
		m_widgets.insert(name, widget);
	}
public slots:
	void dataChanged();
	void completeChanged(bool complete);
private slots:
	void onButtonClicked(QAbstractButton *button);
protected:
	void keyPressEvent(QKeyEvent *e);
private:
	AbstractDataWidget *m_widget;
	bool m_isChanged;
	int m_incompleteWidgets;
	QMultiHash<QString, AbstractDataWidget*> m_widgets;
	QDialogButtonBox *m_buttonsBox;
};

class DefaultDataFormsBackend : public DataFormsBackend
{
	Q_OBJECT
	Q_CLASSINFO("Service", "DataFormsBackend")
public:
	DefaultDataFormsBackend();
	virtual AbstractDataForm *get(const DataItem &item,
								  AbstractDataForm::StandardButtons standartButtons = AbstractDataForm::NoButton,
								  const AbstractDataForm::Buttons &buttons = AbstractDataForm::Buttons());
};

}

#endif // DATAFORMSBACKEND_H
