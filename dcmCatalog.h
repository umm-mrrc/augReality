/*========================================================================*/
#ifndef DCMCATALOG_H
#define DCMCATALOG_H

//#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include <vector>
#include <string>
#include <QDialog>
// DICOM Image File descriptor:
struct dcmImageDesc {
    OFString fname;
    OFString acquisitionNumber;         // (0020,0012)
    OFString instanceNumber;            // (0020,0013)
    OFString sliceLocation;             // (0020,1041)
};
// DICOM Series descriptor:
struct dcmSeriesDesc {
    OFString seriesInstanceUID;         // (0020,000d)
    OFString seriesDescription;         // (0008,103e)
    OFString seriesDate;                // (0008,0021)
    OFString seriesNumber;              // (0020,0011)
    OFString imagePositionPatient;      // (0020,0032)
    float f_imagePositionPatient[3];    //      (0020,0032)
    OFString imageOrientationPatient;   // (0020,0037)
    float f_imageOrientationPatient[6]; //      (0020,0037)
    OFString sliceThickness;            // (0018,0050)
    float f_sliceThickness;             //      (0018,0050)
    OFString pixelSpacing;              // (0028,0030)
    float f_pixelSpacing[2];            //      (0028,0030)
    Uint16 smallestImagePixelValue;     // (0028,0106)
    Uint16 largestImagePixelValue;      // (0028,0107)
    Uint16 rows;                        // (0028,0010)
    Uint16 columns;                     // (0028,0011)
    Uint16 samplesPerPixel;             // (0028,0002)
    Uint16 bitsAllocated;               // (0028,0100)
    Uint16 bitsStored;                  // (0028,0101)
    Uint16 highBit;                     // (0028,0102)
    Uint16 maxAcq;                      // max(0020,0012)
    Uint16 maxInstance;                 // max(0020,0013)
    Uint16 nSlices, nVols;
    Uint16 numberOfFrames;
    std::vector<dcmImageDesc *> dcmImages;
    std::vector<float> sliceLocations;
};
// DICOM Study descriptor:
struct dcmStudyDesc {
    OFString studyInstanceUID;          // (0020,000d)
    OFString studyDescription;          // (0008,1030)
    OFString studyDate;                 // (0008,0020)
    OFString patientName;               // (0010,0010)
    OFString studyID;                   // (0020,0010)
    std::vector<dcmSeriesDesc*> dcmSeries;
};
// Class defining the catalog of all things DICOM...
class dcmCatalog {
private:
    std::vector<dcmStudyDesc *> dcmStudies;
    dcmStudyDesc *findStudy(OFString studyUID);
    dcmSeriesDesc *findSeries(dcmStudyDesc *, OFString seriesUID);
    dcmImageDesc *findImage(dcmSeriesDesc *currSeriesDesc, OFString imageFname);
    void addSliceLocation(std::vector<float>*sliceLocations, float newSliceLocation);
public:
    dcmCatalog();
    ~dcmCatalog();
    void addDcmFile(std::string);
    void addDcmDir(std::string);
    void dumpCatalog();
    QStringList getCatalogList();
    QStringList getStudyList();
    QStringList getSeriesList(std::string studyUID);
    QStringList getImageList(std::string studyUID, std::string seriesUID);
    bool checkIfDcm(std::string);
    int pickStudy(std::string *retStudyUID, std::string *retSeriesUID);
};
#endif // DCMCATALOG_H
