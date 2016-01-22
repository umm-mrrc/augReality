#include "seriesdialog.h"
#include "ui_seriesdialog.h"
#include "iostream"
#include "QStandardItem"
#include "QModelIndexList"
#include "QDirModel"

seriesDialog::seriesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::seriesDialog)
{
    ui->setupUi(this);
    standardModel = new QStandardItemModel ;
}

seriesDialog::~seriesDialog()
{
    delete standardModel;
    delete ui;
}

void seriesDialog::on_buttonBox_accepted()
{
}

void seriesDialog::populateList(QStringList *choices)
{
    myQStandardItem *studyItem = NULL, *seriesItem = NULL;
    std::string pickData;
//    myQStandardItem *rootNode = (myQStandardItem *)standardModel->invisibleRootItem();
    QStandardItem *rootNode = standardModel->invisibleRootItem();
    for (int i = 0; i < choices->size(); ++i) {
        pickData = choices->at(i).toLocal8Bit().constData();
//        std::cout << "seriesDialog: Adding " << pickData << "\n";
        std::vector<int> colons = findCharIndices(pickData, ':');
        if (!pickData.compare(0, 3, "ST:")) {
            std::string studyUID, ptName, studyDate, pickText, uidText;
            studyUID = pickData.substr(colons[0]+1, colons[1]-colons[0]-1);
            ptName = pickData.substr(colons[1]+1, colons[2]-colons[1]-1);
            studyDate = pickData.substr(colons[2]+1);
            pickText = ptName + " (" + studyDate + ")";
            uidText = "ST:" + studyUID;
            studyItem = new myQStandardItem(pickText, uidText);
            rootNode->appendRow(studyItem);
        }
        else {
            std::string studyUID, seriesUID, seriesDesc, seriesDate, seriesNum, pickText, uidText;
            studyUID = pickData.substr(colons[0]+1, colons[1]-colons[0]-1);
            seriesUID = pickData.substr(colons[1]+1, colons[2]-colons[1]-1);
            seriesDesc = pickData.substr(colons[2]+1, colons[3]-colons[2]-1);
            seriesDate = pickData.substr(colons[3]+1, colons[4]-colons[3]-1);
            seriesNum = pickData.substr(colons[4]+1);
            pickText = seriesNum + ": " + seriesDesc;
            uidText = "SR:" + studyUID + ":" + seriesUID;
            seriesItem = new myQStandardItem(pickText, uidText);
            studyItem->appendRow(seriesItem);
        }
    }
//    std::cout << "\nseriesDialog: Last study added = " << studyItem->getUIDS() << "\n";
//    std::cout << "Last series added = " << seriesItem->getUIDS() << "\n";
    //register the model
    ui->seriesTree->setModel(standardModel);
    ui->seriesTree->expandAll();
}

std::vector<int> seriesDialog::findCharIndices(const std::string& sample, char findIt)
{
    std::vector<int> characterLocations;
    for(uint i =0; i < sample.size(); i++)
        if(sample[i] == findIt)
            characterLocations.push_back(i);
    return characterLocations;
}

void seriesDialog::on_seriesTree_doubleClicked(const QModelIndex &index)
{
    myQStandardItem *item = (myQStandardItem *)standardModel->itemFromIndex(index);
    std::string uids = item->getUIDS();
    // Accept the selection if they double-clicked a series...
    if (uids.compare(0, 3, "SR:") == 0) {
        accept();
    }
}

std::string seriesDialog::getSelectedUIDS()
{
    QModelIndex index = ui->seriesTree->currentIndex();
    myQStandardItem *item = (myQStandardItem *)standardModel->itemFromIndex(index);
    std::string uids = item->getUIDS();
//    QDirModel *model = (QDirModel*)ui->seriesTree->model();
//    int row = -1;
//    std::cout << "seriesDialog: getSelectedUIDs:\n";
//    foreach (QModelIndex index, list)
//    {
//        std::cout << "  selected r,c: " << index.row() << ", " << index.column() << "\n";
//        if (index.row()!=row && index.column()==0)
//        {
//            QFileInfo fileInfo = model->fileInfo(index);
//            qDebug() << fileInfo.fileName() << '\n';
//            row = index.row();
//        }
//    }
    return(uids);
}

void seriesDialog::on_seriesTree_activated(const QModelIndex &index)
{
}

void seriesDialog::on_seriesTree_clicked(const QModelIndex &index)
{
}

void seriesDialog::on_buttonBox_rejected()
{
}
