/*========================================================================
default DICOM tags: /usr/include/dcmtk/dcmdata/dcdeftag.h
=========================================================================*/

#include "dcmCatalog.h"

#include <dirent.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <list>
#include "seriesdialog.h"
#include "dcmtk/dcmdata/dctk.h"

// Test whether a file is a DICOM image...
bool dcmCatalog::checkIfDcm(std::string dcmFname)
{
    FILE *ftest;
    ftest = fopen(dcmFname.c_str(), "r");
    if (ftest == NULL)
        return(false);
    if (fseek(ftest, 128, SEEK_SET)) {
        fclose(ftest);
        return(false);
    }
    char dcmTest[4];
    int nb = fread(dcmTest, 1, 4, ftest);
    fclose(ftest);
    if (nb != 4)
        return(false);
    if (strncmp(dcmTest, "DICM", 4) == 0)
        return(true);
    else
        return(false);
}

// Search the study list for a given study...
dcmStudyDesc *dcmCatalog::findStudy(OFString studyInstanceUID)
{
    dcmStudyDesc *retStudyDesc = NULL;
    dcmStudyDesc *currStudyDesc = NULL;
    for (std::vector<dcmStudyDesc*>::iterator it = dcmStudies.begin(); it != dcmStudies.end(); it++) {
        currStudyDesc = (dcmStudyDesc *)(*it);
        if (!(currStudyDesc->studyInstanceUID).compare(studyInstanceUID)) {
            retStudyDesc = currStudyDesc;
            break;
        }
    }
    return(retStudyDesc);
}

// Search a study's series list for a given series...
dcmSeriesDesc *dcmCatalog::findSeries(dcmStudyDesc *currStudyDesc, OFString seriesInstanceUID)
{
    dcmSeriesDesc *retSeriesDesc = NULL;
    dcmSeriesDesc *currSeriesDesc = NULL;
    std::vector<dcmSeriesDesc *>seriesList = currStudyDesc->dcmSeries;
    for (std::vector<dcmSeriesDesc *>::iterator it = seriesList.begin(); it != seriesList.end(); it++) {
        currSeriesDesc = (dcmSeriesDesc *)(*it);
        if (!(currSeriesDesc->seriesInstanceUID).compare(seriesInstanceUID)) {
            retSeriesDesc = currSeriesDesc;
            break;
        }
    }
    return(retSeriesDesc);
}

// Search a series' image list for a give image filename...
dcmImageDesc *dcmCatalog::findImage(dcmSeriesDesc *currSeriesDesc, OFString imageFname)
{
    dcmImageDesc *retImageDesc = NULL;
    dcmImageDesc *currImageDesc = NULL;
    std::vector<dcmImageDesc *>imageList = currSeriesDesc->dcmImages;
    for (std::vector<dcmImageDesc *>::iterator it = imageList.begin(); it != imageList.end(); it++) {
        currImageDesc = (dcmImageDesc *)(*it);
        if (!(currImageDesc->fname).compare(imageFname)) {
            retImageDesc = currImageDesc;
            break;
        }
    }
    return(retImageDesc);
}

dcmCatalog::dcmCatalog()
{
}

dcmCatalog::~dcmCatalog()
{
  std::cout << "dcmCatalog: Deleting dcmCatalog!\n";
}

// Add an image to our catalog if it's a valid DICOM file...
void dcmCatalog::addDcmFile(std::string dcmFname)
{
    OFString studyInstanceUID;
    dcmStudyDesc *currStudy;
    dcmSeriesDesc *currSeries;
    dcmImageDesc *currImage;
    DcmFileFormat dfile;
    OFCondition status;
    DcmDataset *dcmDs;
    // Ignore anything that's obviously a non-DICOM file...
    if (!checkIfDcm(dcmFname)) {
        return;
    }
    // Try to read the DICOM file...
    status = dfile.loadFile(dcmFname.c_str());
    if (!status.good())  {
        std::cout << "dcmCatalog: Error reading DICOM file " << dcmFname << " (" << status.text() << ")" << "\n";
        return;
    }
    dfile.loadAllDataIntoMemory();
    dcmDs = dfile.getDataset();
    //
    // If we haven't seen the study instance UID before
    // Then add the NEW study to the catalog's list of studies...
    dcmDs->findAndGetOFString(DCM_StudyInstanceUID, studyInstanceUID);
    currStudy = findStudy(studyInstanceUID);
    if (currStudy == NULL) {
        currStudy = new dcmStudyDesc;
        currStudy->studyInstanceUID = studyInstanceUID;
        dcmDs->findAndGetOFString(DCM_StudyDescription, currStudy->studyDescription);
        dcmDs->findAndGetOFStringArray(DCM_StudyDate, currStudy->studyDate);
        dcmDs->findAndGetOFStringArray(DCM_PatientName, currStudy->patientName);
        dcmDs->findAndGetOFString(DCM_StudyID, currStudy->studyID);
        dcmStudies.push_back(currStudy);
    }
    //
    // If the series instance UID is unique within the study
    // Then add the NEW series to the study's list of series...
    OFString seriesInstanceUID;
    dcmDs->findAndGetOFString(DCM_SeriesInstanceUID, seriesInstanceUID);
    currSeries = findSeries(currStudy, seriesInstanceUID);
    if (currSeries == NULL) {
        float v0, v1, v2, v3, v4, v5;
        currSeries = new dcmSeriesDesc;
        currSeries->seriesInstanceUID = seriesInstanceUID;
        dcmDs->findAndGetOFString(DCM_SeriesDescription, currSeries->seriesDescription);
        dcmDs->findAndGetOFString(DCM_SeriesDate, currSeries->seriesDate);
        dcmDs->findAndGetOFString(DCM_SeriesNumber, currSeries->seriesNumber);
        dcmDs->findAndGetOFStringArray(DCM_ImagePositionPatient, currSeries->imagePositionPatient);
        sscanf(currSeries->imagePositionPatient.c_str(), "%f\\%f\\%f", &v0, &v1, &v2);
        currSeries->f_imagePositionPatient[0] = v0;
        currSeries->f_imagePositionPatient[1] = v1;
        currSeries->f_imagePositionPatient[2] = v2;
        dcmDs->findAndGetOFStringArray(DCM_ImageOrientationPatient, currSeries->imageOrientationPatient);
        sscanf(currSeries->imageOrientationPatient.c_str(), "%f\\%f\\%f\\%f\\%f\\%f", &v0, &v1, &v2, &v3, &v4, &v5);
        currSeries->f_imageOrientationPatient[0] = v0;
        currSeries->f_imageOrientationPatient[1] = v1;
        currSeries->f_imageOrientationPatient[2] = v2;
        currSeries->f_imageOrientationPatient[3] = v3;
        currSeries->f_imageOrientationPatient[4] = v4;
        currSeries->f_imageOrientationPatient[5] = v5;
        dcmDs->findAndGetOFStringArray(DCM_SliceThickness, currSeries->sliceThickness);
        sscanf(currSeries->sliceThickness.c_str(), "%f", &currSeries->f_sliceThickness);
        dcmDs->findAndGetUint16(DCM_Rows, currSeries->rows);
        dcmDs->findAndGetUint16(DCM_Columns, currSeries->columns);
        dcmDs->findAndGetUint16(DCM_NumberOfFrames, currSeries->numberOfFrames);
        dcmDs->findAndGetUint16(DCM_SamplesPerPixel, currSeries->samplesPerPixel);
        dcmDs->findAndGetUint16(DCM_BitsAllocated, currSeries->bitsAllocated);
        dcmDs->findAndGetUint16(DCM_BitsStored, currSeries->bitsStored);
        dcmDs->findAndGetUint16(DCM_HighBit, currSeries->highBit);
        currSeries->maxAcq = 0;
        currSeries->maxInstance = 0;
        currStudy->dcmSeries.push_back(currSeries);
    }
    //
    // If this is a new image for the series
    // Then add the NEW image to the series's list of images...
    currImage = findImage(currSeries, dcmFname.c_str());
    if (currImage == NULL) {
        currImage = new dcmImageDesc;
        currImage->fname = dcmFname.c_str();
        dcmDs->findAndGetOFString(DCM_AcquisitionNumber, currImage->acquisitionNumber);
        dcmDs->findAndGetOFString(DCM_InstanceNumber, currImage->instanceNumber);
        dcmDs->findAndGetOFStringArray(DCM_SliceLocation, currImage->sliceLocation);
        currSeries->dcmImages.push_back(currImage);
        // Update some series information from the image...
        Uint16 currA = (Uint16)atoi(currImage->acquisitionNumber.c_str());
        Uint16 currI = (Uint16)atoi(currImage->instanceNumber.c_str());
        currSeries->maxAcq = std::max(currSeries->maxAcq, currA);
        currSeries->maxInstance = std::max(currSeries->maxInstance, currI);
        float f_sliceLocation = (float)atof(currImage->sliceLocation.c_str());
        addSliceLocation(&currSeries->sliceLocations, f_sliceLocation);
    }
}

void dcmCatalog::addSliceLocation(std::vector<float>*sliceLocations, float newSliceLocation)
{
    float currSliceLocation;
    for (std::vector<float>::iterator it = sliceLocations->begin(); it != sliceLocations->end(); it++) {
        currSliceLocation = (float)(*it);
        if (currSliceLocation == newSliceLocation) {
            return;
        }
    }
    sliceLocations->push_back(newSliceLocation);
}

// Add all of the DICOM files found in and under a directory to the catalog...
void dcmCatalog::addDcmDir(std::string dcmDir)
{
    if (dcmDir.length() > 0) {
        std::string::iterator it = dcmDir.end() - 1;
        if (*it == '/') dcmDir.erase(it);
    }
    std::list<std::string> dirList;
    dirList.push_back(dcmDir);
    for (std::list<std::string>::iterator currDir = dirList.begin(); currDir != dirList.end(); ++currDir) {
        DIR *pdir = opendir(currDir->c_str());
        if (pdir == NULL) {
            continue;
        }
        std::cout << "dcmCatalog: Cataloging dir " << *currDir << "\n";
        struct dirent *dp = 0;
        while ((dp = readdir(pdir)) != 0) {
            std::string currFile (dp->d_name);
            if (currFile == "." || currFile == "..") {  // skip these special files
                continue;
            }
            std::string fullFname = (*currDir + "/" + currFile);
            // If the current file is a directory
            // Then add it to our list of directories to search
            // Else try to catalog the file as a DICOM image...
            if (dp->d_type == DT_DIR) {
                dirList.push_back(fullFname);
            }
            else {
                addDcmFile(fullFname);
            }
        }
        closedir(pdir);
    }
}

void dcmCatalog::dumpCatalog()
{
    std::cout << "dcmCatalog: Dumping catalog!\n";
    dcmStudyDesc *currStudy;
    dcmSeriesDesc *currSeries;
    for (std::vector<dcmStudyDesc *>::iterator it = dcmStudies.begin(); it != dcmStudies.end(); ++it) {
        currStudy = (dcmStudyDesc *)(*it);
        std::cout << "Study name: " << currStudy->patientName << "\n";
        std::cout << "   UID: " << currStudy->studyInstanceUID << "\n";
        std::cout << "   Desc: " << currStudy->studyDescription << "\n";
        std::cout << "   Study ID: " << currStudy->studyID << "\n";
        // Dump the series associated with the study...
        for (std::vector<dcmSeriesDesc *>::iterator dit = (currStudy->dcmSeries).begin(); dit != (currStudy->dcmSeries).end(); ++dit) {
            currSeries = (dcmSeriesDesc *)(*dit);
            std::cout << "   Series " << currSeries->seriesNumber << ": " << currSeries->seriesDescription << "\n";
            std::cout << "      UID: " << currSeries->seriesInstanceUID << "\n";
            std::cout << "      rows, columns, slices = " << currSeries->rows << ", " << currSeries->columns << ", " << currSeries->sliceLocations.size() << "\n";
            std::cout << "      min, max pixel = " << currSeries->smallestImagePixelValue << ", " << currSeries->largestImagePixelValue << "\n";
            std::cout << "      maxAcq = " <<  currSeries->maxAcq << "\n";
            std::cout << "      maxInstance = " << currSeries->maxInstance << "\n";
            std::cout << "      imagePosition = " << currSeries->f_imagePositionPatient[0] << ", " << currSeries->f_imagePositionPatient[1] << ", " << currSeries->f_imagePositionPatient[2] << "\n";
            std::cout << "      imageOrientation = " << currSeries->f_imageOrientationPatient[0] << ", " << currSeries->f_imageOrientationPatient[1] << ", " << currSeries->f_imageOrientationPatient[2] << "\n";
            std::cout << "           " << currSeries->f_imageOrientationPatient[3] << ", " << currSeries->f_imageOrientationPatient[4] << ", " << currSeries->f_imageOrientationPatient[5] << "\n";
        }
    }
}

// Return a QStringList of all studies and series in the DICOM catalog.
// The individual string entries returned are formatted thusly:
//          ST:<studyUID>:<ptName>:<studyDate>
//          SR:<studyUID>:<seriesUID>:<seriesDesc>:<seriesDate>:<seriesNumber>
QStringList dcmCatalog::getCatalogList()
{
    std::size_t found1, found2;
    QStringList seriesList, catalogList;
    std::string studyData, serData;
    QStringList studyList = getStudyList();
    for (int i = 0; i < studyList.size(); ++i) {
        studyData = studyList.at(i).toLocal8Bit().constData();
        catalogList << studyData.c_str();
        found1 = studyData.find_first_of(":") + 1;
        found2 = studyData.find_first_of(":", found1);
        std::string studyUID = studyData.substr(found1, found2-found1);
        seriesList = getSeriesList(studyUID);
        for (int sei = 0; sei < seriesList.size(); ++sei) {
            serData = seriesList.at(sei).toLocal8Bit().constData();
            catalogList << serData.c_str();
        }
    }
    return(catalogList);
}

// Return a QStringList of ALL studies in the DICOM catalog.
// The individual string entries returned are formatted thusly:
//          ST:<studyUID>:<ptName>:<studyDate>
QStringList dcmCatalog::getStudyList()
{
    dcmStudyDesc *currStudy;
    QStringList studyList;
    // Add each study to the list...
    for (std::vector<dcmStudyDesc *>::iterator it = dcmStudies.begin(); it != dcmStudies.end(); ++it) {
        currStudy = (dcmStudyDesc *)(*it);
        std::string studyUID = currStudy->studyInstanceUID.c_str();
        std::string studyPtName = currStudy->patientName.c_str();
        std::string studyDate = currStudy->studyDate.c_str();
        std::size_t found = studyPtName.find_first_of(":/");
        while (found != std::string::npos) {
            studyPtName[found] = '_';
            found = studyPtName.find_first_of(":/", found+1);
        }
        std::string ident = "ST:" + studyUID + ":" + studyPtName + ":" + studyDate;
        studyList << ident.c_str();
    }
    return(studyList);
}

// Return a QStringList of all series associated with a given study in the DICOM catalog.
// The individual string entries returned are formatted thusly:
//          SR:<studyUID>:<seriesUID>:<seriesDesc>:<seriesDate>:<seriesNumber>
QStringList dcmCatalog::getSeriesList(std::string studyUID)
{
    OFString of_studyUID = studyUID.c_str();
    QStringList seriesList;
    // Find the study...
    dcmStudyDesc *currStudy = findStudy(of_studyUID);
    if (currStudy == NULL) {
        return(seriesList);
    }
    // Return all of the associated series...
    dcmSeriesDesc *currSeries;
    for (std::vector<dcmSeriesDesc *>::iterator it = currStudy->dcmSeries.begin(); it != currStudy->dcmSeries.end(); ++it) {
        currSeries = (dcmSeriesDesc *)(*it);
        std::string seriesUID = currSeries->seriesInstanceUID.c_str();
        std::string seriesDesc = currSeries->seriesDescription.c_str();
        std::string seriesDate = currSeries->seriesDate.c_str();
        std::string seriesNumber = currSeries->seriesNumber.c_str();
        std::size_t found = seriesDesc.find_first_of(":/");
        while (found != std::string::npos) {
            seriesDesc[found] = '_';
            found = seriesDesc.find_first_of(":/", found+1);
        }
        std::string ident = "SR:" + studyUID + ":" + seriesUID + ":" + seriesDesc + ":" + seriesDate + ":" + seriesNumber;
        seriesList << ident.c_str();
    }
    return(seriesList);
}

// Return a QStringList of all image filenames associated with a
//      given study and series in the DICOM catalog
QStringList dcmCatalog::getImageList(std::string studyUID, std::string seriesUID)
{
    OFString of_studyUID = studyUID.c_str();
    OFString of_seriesUID = seriesUID.c_str();
    QStringList imageList;
    // Find the study and series...
    dcmStudyDesc *currStudy = findStudy(of_studyUID);
    if (currStudy == NULL) {
        return(imageList);
    }
    dcmSeriesDesc *currSeries = findSeries(currStudy, of_seriesUID);
    dcmImageDesc *currImage;
    for (std::vector<dcmImageDesc *>::iterator it = currSeries->dcmImages.begin(); it != currSeries->dcmImages.end(); ++it) {
        currImage = (dcmImageDesc *)(*it);
        std::string fname = currImage->fname.c_str();
        imageList << fname.c_str();
    }
    return(imageList);
}

int dcmCatalog::pickStudy(std::string *studyUID, std::string *seriesUID)
{
    //
    // Get a QStringList with all cataloged DICOM studies and series...
    QStringList pickList = getCatalogList();
    //
    // Allow the user to select a DICOM series...
    seriesDialog nd;
    nd.populateList(&pickList);
    if (!nd.exec()) return(0);
    std::string seriesUIDS = nd.getSelectedUIDS();
    if (seriesUIDS.compare(0, 3, "SR:")) return(0);
    //
    // Process their series selection...
    int found1 = seriesUIDS.find_first_of(":") + 1;
    int found2 = seriesUIDS.find_first_of(":", found1) + 1;
    *studyUID = seriesUIDS.substr(found1, found2-found1-1);
    *seriesUID = seriesUIDS.substr(found2);
    return(1);
}
