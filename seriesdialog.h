#ifndef SERIESDIALOG_H
#define SERIESDIALOG_H

#include <QDialog>
#include "QStandardItemModel"
#include "QStringList"
#include "vector"
#include <iostream>

namespace Ui {
class seriesDialog;
}

// QStandardItem with the addition of a text field containing study and possibly series UIDs
class myQStandardItem : public QStandardItem
{
public:
    myQStandardItem(const std::string &listText, const std::string &newUID) : QStandardItem(QString::fromStdString(listText))
    {
        uidText = newUID; //QString::fromStdString(pickText));
    }
    void setUIDS(const std::string text)
    {
        uidText = text;
    }

    std::string getUIDS()
    {
        return(uidText);
    }

private:
    std::string uidText;
};


class seriesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit seriesDialog(QWidget *parent = 0);
    ~seriesDialog();
    void populateList(QStringList *choices);
    std::string getSelectedUIDS();

private slots:
    void on_buttonBox_accepted();
    void on_seriesTree_doubleClicked(const QModelIndex &index);

    void on_seriesTree_activated(const QModelIndex &index);

    void on_seriesTree_clicked(const QModelIndex &index);

    void on_buttonBox_rejected();

private:
    Ui::seriesDialog *ui;
    QStandardItemModel *standardModel;
    std::vector<int> findCharIndices(const std::string&, char);
};

#endif // SERIESDIALOG_H
