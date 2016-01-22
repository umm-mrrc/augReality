#include "vtkdicom.h"
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include "vtkImageData.h"
#include "vtkMath.h"

// Allocate some variables...
vtkDicom::vtkDicom()
{
    vtk_reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    dcm2vtk = vtkSmartPointer<vtkMatrix4x4>::New();
    vtk2dcm = vtkSmartPointer<vtkMatrix4x4>::New();
    imageData = vtkSmartPointer<vtkImageData>::New();
}

// Load DICOM files from a directory...
void vtkDicom::load_dcmDirectory(std::string dirName)
{
    std::cout << "vtkdicom: Loading DICOM from " << dirName << "\n";
    // Read the volume from the directory...
    vtk_reader->SetDirectoryName("");
    vtk_reader->SetDirectoryName(dirName.c_str());
    vtk_reader->Update();
    // Load the DICOM information...
    std::cout << "vtkdicom: Loading DICOMinfo\n";
    loadDicomInfo();
    std::cout << "vtkdicom: Done\n";
}

// Load our vtkImageData object from a list of DICOM image filenames...
// http://www.vtk.org/pipermail/vtkusers/2007-August/042635.html
void vtkDicom::load_dcmFiles(QStringList dcm_fileList) {
    DcmFileFormat dcm;
    std::string imgFname;
    std::cout << "vtkdicom: Files/Images(?): " << dcm_fileList.size() << "\n";
    for (int sei = 0; sei < dcm_fileList.size(); ++sei) {
        imgFname = dcm_fileList.at(sei).toLocal8Bit().constData();
        OFCondition status = dcm.loadFile(imgFname.c_str());
        if (!status.good()) {
            std::cout << "    vtkdicom: Error: cannot read file (" << status.text() << ")" << "\n";
            return;
        }
        if (sei == 0) {
            OFString acquisitionNumber, instanceNumber, imagePositionPatient, patientsName;
            DcmDataset *dcmDs = dcm.getDataset();
            dcmDs->findAndGetOFStringArray(DCM_ImagePositionPatient, imagePositionPatient);
            dcmDs->findAndGetOFString(DCM_AcquisitionNumber, acquisitionNumber);
            dcmDs->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
            dcmDs->findAndGetOFString(DCM_PatientName, patientsName);
            std::cout << "vtkdicom: I#, IPP: " << instanceNumber << " - " << imagePositionPatient << "\n";
        }
        dcm.loadAllDataIntoMemory();
        const unsigned short* p = NULL;
        dcm.getDataset()->findAndGetUint16Array(DCM_PixelData, p);

    }
}

// Load the dcmInfo structure from the DICOM data...
void vtkDicom::loadDicomInfo() {
    float *p_lasPos = vtk_reader->GetImagePositionPatient();
    float *p_orientation = vtk_reader->GetImageOrientationPatient();
    double *p_spacing = vtk_reader->GetDataSpacing();
    int *p_dataExtent = vtk_reader->GetDataExtent();
    vtkImageData *p_imdataBrain = vtk_reader->GetOutput();//    if (!activeTracking) {
    p_imdataBrain->GetScalarRange(dataRange);
    float *p_xdir = &p_orientation[0];
    float *p_ydir = &p_orientation[3];
    float p_zdir[3];
    vtkMath::Cross(p_xdir, p_ydir, p_zdir);
    for (int i = 0; i < 3; i++) {
       xDir[i] = p_xdir[i];
       yDir[i] = p_ydir[i];
       zDir[i] = p_zdir[i];
       lasPos[i] = p_lasPos[i];
    }
    // Get the image's field of view...
    double ncm1 = p_dataExtent[1] - p_dataExtent[0];
    double nrm1 = p_dataExtent[3] - p_dataExtent[2];
    double nsm1 = p_dataExtent[5] - p_dataExtent[4];
    xFOV = ncm1 * p_spacing[0];
    yFOV = nrm1 * p_spacing[1];
    zFOV = nsm1 * p_spacing[2];
    // Get the coordinates of the corner points of the DICOM volume...
    for (int i = 0; i < 3; i++) {
        lpsPos[i] = lasPos[i] + p_spacing[i] * xDir[i] * ncm1;
        laiPos[i] = lasPos[i] + p_spacing[i] * yDir[i] * nrm1;
        rasPos[i] = lasPos[i] + p_spacing[i] * zDir[i] * nsm1;
        lpiPos[i] = lasPos[i] + p_spacing[i] * (yDir[i] * nrm1 + xDir[i] * ncm1);
        rpsPos[i] = lasPos[i] + p_spacing[i] * (zDir[i] * nsm1 + xDir[i] * ncm1);
        raiPos[i] = lasPos[i] + p_spacing[i] * (yDir[i] * nrm1 + zDir[i] * nsm1);
        rpiPos[i] = lasPos[i] + p_spacing[i] * (zDir[i] * nsm1 + xDir[i] * ncm1 + yDir[i] * nrm1);
    }
    // Get the VTK to DICOM transformation matrix...
    for (int i = 0; i < 3; i++) {
        vtk2dcm->Element[i][0] = xDir[i];
        vtk2dcm->Element[i][1] = -yDir[i];
        vtk2dcm->Element[i][2] = -zDir[i];
        vtk2dcm->Element[i][3] = raiPos[i];
    }
    // Invert the VTK to DICOM transformation to get the DICOM to VTK transformation matrix...
    vtk2dcm->Invert(vtk2dcm, dcm2vtk);
}

// Debug dump the data...
void vtkDicom::dbDump()
{
    std::cout << "vtkDicom::dbDump()\n";
    vtk_reader->Print(std::cout);
    std::cout << "vtkdicom: xFOV, yFOV, zFOV: " << xFOV << ", " << yFOV << ", " << zFOV << "\n";
//    // Print our matrices...
//    std::cout << "\nvtk2dcm: \n";
//    vtk2dcm->Print(std::cout);
//    std::cout << "\ndcm2vtk: \n";
//    dcm2vtk->Print(std::cout);
    // Print out our corner points...
    double outp[3];
    matMultPt(dcm2vtk, raiPos, outp);
    std::cout << "vtkdicom: raiPos_dcm = [ " << raiPos[0] << " " << raiPos[1] << " " << raiPos[2] << "];\n";
    std::cout << "raiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dcm2vtk, laiPos, outp);
    std::cout << "laiPos_dcm = [ " << laiPos[0] << " " << laiPos[1] << " " << laiPos[2] << "];\n";
    std::cout << "laiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dcm2vtk, rasPos, outp);
    std::cout << "rasPos_dcm = [ " << rasPos[0] << " " << rasPos[1] << " " << rasPos[2] << "];\n";
    std::cout << "rasPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dcm2vtk, lasPos, outp);
    std::cout << "lasPos_dcm = [ " << lasPos[0] << " " << lasPos[1] << " " << lasPos[2] << "];\n";
    std::cout << "lasPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dcm2vtk, rpiPos, outp);
    std::cout << "rpiPos_dcm = [ " << rpiPos[0] << " " << rpiPos[1] << " " << rpiPos[2] << "];\n";
    std::cout << "rpiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dcm2vtk, lpiPos, outp);
    std::cout << "lpiPos_dcm = [ " << lpiPos[0] << " " << lpiPos[1] << " " << lpiPos[2] << "];\n";
    std::cout << "lpiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dcm2vtk, rpsPos, outp);
    std::cout << "rpsPos_dcm = [ " << rpsPos[0] << " " << rpsPos[1] << " " << rpsPos[2] << "];\n";
    std::cout << "rpsPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dcm2vtk, lpsPos, outp);
    std::cout << "lpsPos_dcm = [ " << lpsPos[0] << " " << lpsPos[1] << " " << lpsPos[2] << "];\n";
    std::cout << "lpsPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
}

void vtkDicom::matMultPt(vtkSmartPointer<vtkMatrix4x4> mat4x4, double *inp3, double *outp3)
{
    double inp4[4];
    double outp4[4];
    for (int i = 0; i < 3; i++)
        inp4[i] = inp3[i];
    inp4[3] = 1;
    mat4x4->MultiplyPoint(inp4, outp4);
    for (int i = 0; i < 3; i++)
        outp3[i] = outp4[i];
}

void vtkDicom::dumpDicomFile(std::string imgFname)
{
    DcmFileFormat dfile;
    OFCondition status = dfile.loadFile(imgFname.c_str());
    if (status.good()) {
        OFString acquisitionNumber, instanceNumber, imagePositionPatient, patientsName;
        dfile.loadAllDataIntoMemory();
        DcmDataset *dcmDs = dfile.getDataset();
        dcmDs->findAndGetOFStringArray(DCM_ImagePositionPatient, imagePositionPatient);
        dcmDs->findAndGetOFString(DCM_AcquisitionNumber, acquisitionNumber);
        dcmDs->findAndGetOFString(DCM_InstanceNumber, instanceNumber);
        dcmDs->findAndGetOFString(DCM_PatientName, patientsName);
        std::cout << "    " << instanceNumber << " - " << imagePositionPatient << "\n";
    } else {
        std::cout << "    Error: cannot read file (" << status.text() << ")" << "\n";
    }
}
