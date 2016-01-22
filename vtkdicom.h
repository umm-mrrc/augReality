#ifndef VTKDICOM_H
#define VTKDICOM_H

#include "qstringlist.h"
#include "vtkSmartPointer.h"
#include "vtkMatrix4x4.h"
#include "vtkVolume.h"
#include "vtkDICOMImageReader.h"
#include "vtkImageData.h"

class vtkDicom
{
public:
    vtkDicom();
    void load_dcmDirectory(std::string dirName);
    void load_dcmFiles(QStringList dcm_fileList);
    void dumpDicomFile(std::string imgFname);
    vtkSmartPointer<vtkVolume> get_vtkVolume();
    void dbDump();
    vtkSmartPointer<vtkMatrix4x4> get_dcm2vtk();
    vtkSmartPointer<vtkMatrix4x4> get_vtk2dcm();

private:
    double xDir[3], yDir[3], zDir[3];
    double xFOV, yFOV, zFOV;
    double lasPos[3];
    double raiPos[3];
    double laiPos[3];
    double rasPos[3];
    double rpiPos[3];
    double lpiPos[3];
    double lpsPos[3];
    double rpsPos[3];
    double dataRange[2];
    //
    void loadDicomInfo() ;
    void matMultPt(vtkSmartPointer<vtkMatrix4x4> mat4x4, double *inp3, double *outp3);
    //
    // Transform matrices to map between DICOM and VTK coordinates...
    //  NOTE: xf_pt = XF_MAT * pt
    vtkSmartPointer<vtkMatrix4x4> dcm2vtk, vtk2dcm;
    vtkSmartPointer<vtkVolume> vtk_volume;
    vtkSmartPointer<vtkDICOMImageReader> vtk_reader;
    vtkSmartPointer<vtkImageData> imageData;
};

#endif // VTKDICOM_H
