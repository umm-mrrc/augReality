#include "augreality.h"
#include "ui_augreality.h"
#include <vtkDICOMImageReader.h>
#include <vtkSmartPointer.h>
#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCornerAnnotation.h"
#include "vtkTextProperty.h"
#include "vtkImagePlaneWidget.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkSLCReader.h"
#include "vtkImageViewer2.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkDICOMImageReader.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkRendererCollection.h"
#include "vtkSphereSource.h"
#include "vtkImageData.h"
#include "vtkProperty.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolumeTextureMapper3D.h"
#include "vtkCylinderSource.h"
#include "vtkLineSource.h"
#include "vtkTubeFilter.h"
#include "vtkImageChangeInformation.h"
#include "vtkMath.h"
#include "dirent.h"
#include "vtkMatrix3x3.h"
#include "QMessageBox"

#define PI 3.14159265359

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"

MyTimer *timer;

void augReality::imgUpdate()
{
    // Update the connection status...
//    std::cout << "imgUpdate\n";
    int ecs = p_endoQuery->getConnectStatus();
    if (ecs != currEndoStatus) {
//        std::cout << "augReality: Connect status " << ecs << "\n";
        currEndoStatus = ecs;
        if (currEndoStatus == ECS_IDLE) {
            ui->endoConnect->setText("Connect");
            ui->endoConnect->setStyleSheet("color: black");
            ui->endoConnect->setDisabled(false);
        } else if (currEndoStatus == ECS_CONNECTING) {
            ui->endoConnect->setText("Connecting");
            ui->endoConnect->setStyleSheet("color: red");
            ui->endoConnect->setDisabled(true);
        } else if (currEndoStatus == ECS_CONNECTED) {
            ui->endoConnect->setText("Disconnect");
            ui->endoConnect->setStyleSheet("color: green");
            ui->endoConnect->setDisabled(false);
        }
    }
    //
    int sensorNum = 1;
    sensorPos sPos;
    int res = p_endoQuery->getEndoPos(sensorNum, &sPos);
    setSensorPos(sensorNum, &sPos);
    setSensorVisibility(sensorNum, true);
    return;
}

augReality::augReality(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::augReality)
{
// NOTE: EndoScout [0] and [1] are negative for this orientation!
    double nosePos0[3] = {-10.38, -132.25, -33.26};
    double noseNorm0[3] = {-0.0333, 0.4683, 0.8829};
    ui->setupUi(this);

    // Initialize global variables...
    initVars();

    // Initialize the VTK display 'stuff'...
    initVtk();

    // Create our temporary directory so we can load our DICOM images...
    char homeDir[1024];
    char *gcres = getcwd(homeDir, 1024);
    dicomTempDir = homeDir;
    dicomTempDir += "/tempDir";
    std::cout << "Homedir : " << homeDir << "\n";
    std::cout << "dicomTempDir: " << dicomTempDir << "\n";
    int res = mkdir(dicomTempDir.c_str(), ACCESSPERMS);
    std::cout << "mkdir result: " << res << "\n";


    // Load and display an initial DICOM volume...
//    loadDicomDirectory("/media/data_gauss/augmentedReality/srrtest_09162015/DICOM");
    loadDicomDirectory("/media/data_gauss/augmentedReality/testStudies/ar_01212016/dccvTempdir");
    setDicomVisibility(true);

#if 0
    // Get some important points...
    //double defNorm[3] = {0.1430, 0.0880, 0.9858};
    double volCenter[3], frontCenter[3], topCenter[3], leftCenter[3];
    for (int i = 0; i < 3; i++) {
        volCenter[i] = (dicomInfo.rasPos[i] + dicomInfo.lpiPos[i]) / 2;
        frontCenter[i] = (dicomInfo.rasPos[i] + dicomInfo.laiPos[i]) / 2;
        topCenter[i] = (dicomInfo.rasPos[i] + dicomInfo.lpsPos[i]) / 2;
        leftCenter[i] = (dicomInfo.lasPos[i] + dicomInfo.lpiPos[i]) / 2;
    }

    // Initialize clipping...
    setClipPlane(topCenter, volCenter, 0);
    setClipPlaneActive(true);

    // Initalize and display the sensors...
    setSensorPos(0, topCenter, dicomInfo.zDir);
    setSensorPos(1, nosePos0, noseNorm0);
//    setSensorPos(1, raoPos, raoNorm);
    setSensorVisibility(0, true);
    setSensorVisibility(1, true);

    // Initialize and display the target position...
    setTargetSourcePos(topCenter);
    setTargetDestPos(volCenter);
    setTargetVisibility(true);
    setTrajectoryVisibility(true);
#endif

    //
    // Reset...
    vtk_renderer->ResetCamera();
    // Render
    updateDisplay();


    //==============================
    // Add DICOM image directories...
    dcmCat.addDcmDir("/media/data_gauss/augmentedReality/testStudies");
//    dcmCat.dumpCatalog();
    //==============================

    // Start up a thread to query the EndoScout...
    p_endoQuery = new endoQuery;
    currEndoStatus = p_endoQuery->getConnectStatus();
    std::cout << "augreality: initial endoStatus: " << currEndoStatus << "\n";

    // Finally, set up a timer to watch for stuff to do...
    timer = new MyTimer();
    timer->start(this);

}

// Initialize our global variables to something 'reasonable'...
void augReality::initVars()
{
    numSensors = 1;
    activeTracking = false;
    test = 0;
    displayChanged = false;
    dicomInfo.dcm2vtk = vtkSmartPointer<vtkMatrix4x4>::New();
    dicomInfo.vtk2dcm = vtkSmartPointer<vtkMatrix4x4>::New();
    clipInfo.depth = 0.0;
    clipInfo.active = false;
    dicomInfo.visible = false;
    for (int i=0; i < MAX_SENSOR; i++) {
        sensorInfo[i].visible = false;
    }
    targetInfo.targetVisible = false;
    targetInfo.trajectoryVisible = false;
    endoConnected = false;
}

// Initialize all of the VTK display stuff...
void augReality::initVtk()
{
    // Add a renderer to the 3D volumetric vtkRenderWindow...
    vtk_renderWindow = ui->render3D->GetRenderWindow();
    vtk_renderer = vtkSmartPointer<vtkRenderer>::New();
    vtk_renderWindow->AddRenderer(vtk_renderer);
    // Set up the volumetric Dicom image display...
    dicomInfo.vtk_volume = vtkSmartPointer<vtkVolume>::New();
    dicomInfo.vtk_mapper = vtkSmartPointer<vtkVolumeTextureMapper2D>::New();
    dicomInfo.vtk_clipPlane = vtkSmartPointer<vtkPlane>::New();
    dicomInfo.vtk_reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    //
    dicomInfo.vtk_mapper->SetInput(dicomInfo.vtk_reader->GetOutput());
    dicomInfo.vtk_volume->SetVisibility(false);
    dicomInfo.vtk_volume->SetMapper(dicomInfo.vtk_mapper);
    vtkSmartPointer<vtkVolumeProperty> dcm3DVolProp = vtkSmartPointer<vtkVolumeProperty>::New();
    dcm3DVolProp->SetInterpolationTypeToLinear();
    dicomInfo.vtk_lut = vtkSmartPointer<vtkColorTransferFunction>::New();
    dcm3DVolProp->SetColor(dicomInfo.vtk_lut);
    vtkSmartPointer<vtkPiecewiseFunction> dcm3DOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    double tVal = .75;
    int background = 75;
    dcm3DOpacity->AddPoint(0, 0.0);
    dcm3DOpacity->AddPoint(background-1, 0.0);
    dcm3DOpacity->AddPoint(background, tVal);
    dcm3DOpacity->AddPoint(255, tVal);
    dcm3DVolProp->SetScalarOpacity(dcm3DOpacity);
    dicomInfo.vtk_volume->SetProperty(dcm3DVolProp);
    vtk_renderer->AddViewProp(dicomInfo.vtk_volume);
    // Set up to graphically display the positions of multiple sensors...
    for (int sensorNum = 0; sensorNum < MAX_SENSOR; sensorNum++) {
        sensorInfo[sensorNum].vtk_posActor = vtkSmartPointer<vtkActor>::New();
        sensorInfo[sensorNum].vtk_posActor->SetVisibility(false);
        sensorInfo[sensorNum].vtk_posActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
        vtkSmartPointer<vtkPolyDataMapper> posMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        sensorInfo[sensorNum].vtk_posMarker = vtkSmartPointer<vtkSphereSource>::New();
        sensorInfo[sensorNum].vtk_posMarker->SetRadius(4);
        sensorInfo[sensorNum].vtk_posMarker->SetThetaResolution(90);
        sensorInfo[sensorNum].vtk_posMarker->SetPhiResolution(90);
        posMapper->SetInput(sensorInfo[sensorNum].vtk_posMarker->GetOutput());
        sensorInfo[sensorNum].vtk_posActor->SetMapper(posMapper);
        vtk_renderer->AddViewProp(sensorInfo[sensorNum].vtk_posActor);
        sensorInfo[sensorNum].vtk_normActor = vtkSmartPointer<vtkActor>::New();
        sensorInfo[sensorNum].vtk_normActor->SetVisibility(false);
        sensorInfo[sensorNum].vtk_normActor->GetProperty()->SetColor(0.0, 1.0, 0.0);
        vtkSmartPointer<vtkPolyDataMapper> normMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        vtkSmartPointer<vtkTubeFilter> normFilter = vtkSmartPointer<vtkTubeFilter>::New();
        normFilter->SetRadius(1.5); //default is .5
        normFilter->SetNumberOfSides(50);
        sensorInfo[sensorNum].vtk_normLine = vtkSmartPointer<vtkLineSource>::New();
        normFilter->SetInput(sensorInfo[sensorNum].vtk_normLine->GetOutput());
        normMapper->SetInput(normFilter->GetOutput());
        sensorInfo[sensorNum].vtk_normActor->SetMapper(normMapper);
        vtk_renderer->AddViewProp(sensorInfo[sensorNum].vtk_normActor);
    }
    // Set up to graphically display the target point and trajectory...
    targetInfo.vtk_targetActor = vtkSmartPointer<vtkActor>::New();
    targetInfo.vtk_targetActor->SetVisibility(false);
    targetInfo.vtk_targetActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    vtkSmartPointer<vtkPolyDataMapper> targetMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    targetInfo.vtk_targetMarker = vtkSmartPointer<vtkSphereSource>::New();
    targetInfo.vtk_targetMarker->SetRadius(4);
    targetInfo.vtk_targetMarker->SetThetaResolution(90);
    targetInfo.vtk_targetMarker->SetPhiResolution(90);
    targetMapper->SetInput(targetInfo.vtk_targetMarker->GetOutput());
    targetInfo.vtk_targetActor->SetMapper(targetMapper);
    vtk_renderer->AddViewProp(targetInfo.vtk_targetActor);
    targetInfo.vtk_trajectoryActor = vtkSmartPointer<vtkActor>::New();
    targetInfo.vtk_trajectoryActor->SetVisibility(false);
    targetInfo.vtk_trajectoryActor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    vtkSmartPointer<vtkPolyDataMapper> trajectoryMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkTubeFilter> trajectoryFilter = vtkSmartPointer<vtkTubeFilter>::New();
    trajectoryFilter->SetRadius(1.5); //default is .5
    trajectoryFilter->SetNumberOfSides(50);
    targetInfo.vtk_trajectoryLine = vtkSmartPointer<vtkLineSource>::New();
    trajectoryFilter->SetInput(targetInfo.vtk_trajectoryLine->GetOutput());
    trajectoryMapper->SetInput(trajectoryFilter->GetOutput());
    targetInfo.vtk_trajectoryActor->SetMapper(trajectoryMapper);
    vtk_renderer->AddViewProp(targetInfo.vtk_trajectoryActor);
}

// Load a new DICOM volume from a directory...
void augReality::loadDicomDirectory(std::string dcmFname)
{
    // Read the volume from the directory...
    dicomInfo.vtk_reader->SetDirectoryName("");
    dicomInfo.vtk_reader->SetDirectoryName(dcmFname.c_str());
    dicomInfo.vtk_reader->Update();
    std::cout << "augreality: ???\n";
    dicomInfo.vtk_reader->SetDataSpacing(5.0, 1.0, 1.0);
    dicomInfo.vtk_reader->Update();

    // Load the dcmInfo structure from the DICOM data...
    loadDicomInfo();
    // Set the image gray scale LUT to display [0,max] as [black, white]...
    dicomInfo.vtk_lut->RemoveAllPoints();
    dicomInfo.vtk_lut->AddRGBPoint(0, 0, 0, 0);
    dicomInfo.vtk_lut->AddRGBPoint(dicomInfo.dataRange[1], 1, 1, 1);
    if (dicomInfo.visible) {
        displayChanged = true;
    }
}

// Load the dcmInfo structure from the DICOM data...
void augReality::loadDicomInfo() {
    float *lasPos = dicomInfo.vtk_reader->GetImagePositionPatient();
    float *orientation = dicomInfo.vtk_reader->GetImageOrientationPatient();
    double *spacing = dicomInfo.vtk_reader->GetDataSpacing();
    int *dataExtent = dicomInfo.vtk_reader->GetDataExtent();
    vtkImageData *imdataBrain = dicomInfo.vtk_reader->GetOutput();//    if (!activeTracking) {
    imdataBrain->GetScalarRange(dicomInfo.dataRange);
    float *xdir = &orientation[0];
    float *ydir = &orientation[3];
    float zdir[3];
    vtkMath::Cross(xdir, ydir, zdir);
    for (int i = 0; i < 3; i++) {
       dicomInfo.xDir[i] = xdir[i];
       dicomInfo.yDir[i] = ydir[i];
       dicomInfo.zDir[i] = zdir[i];
       dicomInfo.lasPos[i] = lasPos[i];
    }
    // Get the image's field of view...
    double ncm1 = dataExtent[1] - dataExtent[0];
    double nrm1 = dataExtent[3] - dataExtent[2];
    double nsm1 = dataExtent[5] - dataExtent[4];
    dicomInfo.xFOV = ncm1 * spacing[0];
    dicomInfo.yFOV = nrm1 * spacing[1];
    dicomInfo.zFOV = nsm1 * spacing[2];
    // Get the coordinates of the corner points of the DICOM volume...
    for (int i = 0; i < 3; i++) {
        dicomInfo.lpsPos[i] = dicomInfo.lasPos[i] + spacing[i] * xdir[i] * ncm1;
        dicomInfo.laiPos[i] = dicomInfo.lasPos[i] + spacing[i] * ydir[i] * nrm1;
        dicomInfo.rasPos[i] = dicomInfo.lasPos[i] + spacing[i] * zdir[i] * nsm1;
        dicomInfo.lpiPos[i] = dicomInfo.lasPos[i] + spacing[i] * (ydir[i] * nrm1 + xdir[i] * ncm1);
        dicomInfo.rpsPos[i] = dicomInfo.lasPos[i] + spacing[i] * (zdir[i] * nsm1 + xdir[i] * ncm1);
        dicomInfo.raiPos[i] = dicomInfo.lasPos[i] + spacing[i] * (ydir[i] * nrm1 + zdir[i] * nsm1);
        dicomInfo.rpiPos[i] = dicomInfo.lasPos[i] + spacing[i] * (zdir[i] * nsm1 + xdir[i] * ncm1 + ydir[i] * nrm1);
    }
    // Get the VTK to DICOM transformation matrix...
    for (int i = 0; i < 3; i++) {
        dicomInfo.vtk2dcm->Element[i][0] = xdir[i];
        dicomInfo.vtk2dcm->Element[i][1] = -ydir[i];
        dicomInfo.vtk2dcm->Element[i][2] = -zdir[i];
        dicomInfo.vtk2dcm->Element[i][3] = dicomInfo.raiPos[i];
    }
    // Invert the VTK to DICOM transformation to get the DICOM to VTK transformation matrix...
    dicomInfo.vtk2dcm->Invert(dicomInfo.vtk2dcm, dicomInfo.dcm2vtk);
}

void augReality::dbDump()
{
    std::cout << "augReality::dbDump()\n";
    dicomInfo.vtk_reader->Print(std::cout);
    // Write stuff out for debugging...
    std::cout << "vtkCoord.xFOV, vtkCoord.yFOV, vtkCoord.zFOV: " << dicomInfo.xFOV << ", " << dicomInfo.yFOV << ", " << dicomInfo.zFOV << "\n";
    // Print our matrices...
    std::cout << "\ndicomInfo.vtk2dcm: \n";
    dicomInfo.vtk2dcm->Print(std::cout);
    std::cout << "\ndicomInfo.dcm2vtk: \n";
    dicomInfo.dcm2vtk->Print(std::cout);
    // Print out our corner points...
    double outp[3];
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.raiPos, outp);
    std::cout << "raiPos_dcm = [ " << dicomInfo.raiPos[0] << " " << dicomInfo.raiPos[1] << " " << dicomInfo.raiPos[2] << "];\n";
    std::cout << "raiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.laiPos, outp);
    std::cout << "laiPos_dcm = [ " << dicomInfo.laiPos[0] << " " << dicomInfo.laiPos[1] << " " << dicomInfo.laiPos[2] << "];\n";
    std::cout << "laiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.rasPos, outp);
    std::cout << "rasPos_dcm = [ " << dicomInfo.rasPos[0] << " " << dicomInfo.rasPos[1] << " " << dicomInfo.rasPos[2] << "];\n";
    std::cout << "rasPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.lasPos, outp);
    std::cout << "lasPos_dcm = [ " << dicomInfo.lasPos[0] << " " << dicomInfo.lasPos[1] << " " << dicomInfo.lasPos[2] << "];\n";
    std::cout << "lasPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.rpiPos, outp);
    std::cout << "rpiPos_dcm = [ " << dicomInfo.rpiPos[0] << " " << dicomInfo.rpiPos[1] << " " << dicomInfo.rpiPos[2] << "];\n";
    std::cout << "rpiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.lpiPos, outp);
    std::cout << "lpiPos_dcm = [ " << dicomInfo.lpiPos[0] << " " << dicomInfo.lpiPos[1] << " " << dicomInfo.lpiPos[2] << "];\n";
    std::cout << "lpiPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.rpsPos, outp);
    std::cout << "rpsPos_dcm = [ " << dicomInfo.rpsPos[0] << " " << dicomInfo.rpsPos[1] << " " << dicomInfo.rpsPos[2] << "];\n";
    std::cout << "rpsPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
    matMultPt(dicomInfo.dcm2vtk, dicomInfo.lpsPos, outp);
    std::cout << "lpsPos_dcm = [ " << dicomInfo.lpsPos[0] << " " << dicomInfo.lpsPos[1] << " " << dicomInfo.lpsPos[2] << "];\n";
    std::cout << "lpsPos_vtk = [ " << outp[0] << " " << outp[1] << " " << outp[2] << "];\n";
}

augReality::~augReality()
{
    // Delete the temporary directory we created for the DICOM images...
    int res = rmdir(dicomTempDir.c_str());
    if (res) {
        std::cout << "augreality: rmdir failure: " << errno << "\n";
    }
    //
    std::cout << "Bye bye\n";
    delete ui;
}

void augReality::dumpDicomReader(vtkSmartPointer<vtkDICOMImageReader> dicomReader)
{
    std::cout << "augreality: ********DICOM INFO: \n";
    dicomReader->Print(std::cout);
    float *io = dicomReader->GetImageOrientationPatient();
    std::cout << " Image orientation: ";
    for (int dd = 0; dd < 3; dd++) std::cout << io[dd] << " ";
    std::cout << "\n";
    io = dicomReader->GetImagePositionPatient();
    std::cout << " Image position: ";
    for (int dd = 0; dd < 3; dd++) std::cout << io[dd] << " ";
    std::cout << "\n";
    int *de = dicomReader->GetDataExtent();
    std::cout << " Data extent: ";
    for (int dd = 0; dd < 6; dd++) std::cout << de[dd] << " ";
    std::cout << "\n";
//    // Test the DICOM toolkit...
//    std::string dcmFN = "/media/data_gauss/augmentedReality/srrtest_09162015/DICOM/STEVE_TEST.MR.ROBINMEDICAL_MAPPING_ESPREE.0002.0192.2015.09.16.12.57.11.765625.73651732.IMA";
//    DcmFileFormat fileformat;
//    std::cout << "Reading DICOM " << dcmFN.c_str() << "\n";
//    OFCondition status = fileformat.loadFile(dcmFN.c_str());
//    if (status.good()) {
//       OFString patientsName;
//       if (fileformat.getDataset()->findAndGetOFString(DCM_PatientName, patientsName).good()) {
//          std::cout << "Patient's Name: " << patientsName << "\n";
//       } else
//         std::cout << "Error: cannot access Patient's Name!" << "\n";
//    } else {
//        std::cout << "Error: cannot read DICOM file (" << status.text() << ")" << "\n";
//    }
}

// Clip depth scroll bar...
void augReality::on_depthScrollBar_valueChanged(int value)
{
    double depth;
    depth = dicomInfo.yFOV * (50.0 - (double)value) / 100.0;
    setClipPlaneDepth(depth);
    updateDisplay();
}

void augReality::mapDcmVector2Vtk(double *dcmVect , double *vtkVect)
{
    // Vectors need to be translated to raiPos before transforming...
    double inVect[3];
    for (int i = 0; i < 3; i++) {
        inVect[i] = dcmVect[i] + dicomInfo.raiPos[i];
    }
    matMultPt(dicomInfo.dcm2vtk, inVect, vtkVect);
}

void augReality::matMultPt(vtkSmartPointer<vtkMatrix4x4> mat4x4, double *inp3, double *outp3)
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

// Set the visibility of the DICOM volume...
void augReality::setDicomVisibility(bool visible)
{
    if (visible == dicomInfo.visible) return;
    dicomInfo.visible = visible;
    dicomInfo.vtk_volume->SetVisibility(visible);
    displayChanged = true;
}

void augReality::setClipPlane(double *source, double *dest, double depth)
{
    // Save the clip plane values...
    clipInfo.depth = depth;
    for (int i = 0; i < 3; i++) {
        clipInfo.sourcePos[i] = source[i];
        clipInfo.destPos[i] = dest[i];
    }
    updateClipPlane();
}

// Set the clip plane origin position...
void augReality::setClipPlaneSource(double *source)
{
    for (int i = 0; i < 3; i++) {
        clipInfo.sourcePos[i] = source[i];
    }
    updateClipPlane();
}

// Set the clip plane end position...
void augReality::setClipPlaneDest(double *dest)
{
    for (int i = 0; i < 3; i++) {
        clipInfo.destPos[i] = dest[i];
    }
    updateClipPlane();
}

// Set the clip plane depth...
void augReality::setClipPlaneDepth(double depth)
{
    clipInfo.depth = depth;
    updateClipPlane();
}

// Set the clip plane according to the DICOM coordinates and the depth...
void augReality::updateClipPlane()
{
    double dcmClipNormal[3];
    double dcmClipOrigin[3];
    double vtkClipOrigin[3];
    double vtkClipNormal[3];
    // Get the clip normal vector...
    for (int i = 0; i < 3; i++) {
        dcmClipNormal[i] = clipInfo.destPos[i] - clipInfo.sourcePos[i];
    }
    double nl = sqrt(pow(dcmClipNormal[0], 2.0) + pow(dcmClipNormal[1], 2.0) + pow(dcmClipNormal[2], 2.0));
    for (int i = 0; i < 3; i++) {
        dcmClipNormal[i] /= nl;
    }
    // Get the origin of the clip plane, taking the depth into account...
    for (int i = 0; i < 3; i++) {
        dcmClipOrigin[i] = clipInfo.destPos[i] + dcmClipNormal[i] * clipInfo.depth;
    }
    // Map the origin of the clip plane from DICOM to VTK coordinates...
    matMultPt(dicomInfo.dcm2vtk, dcmClipOrigin, vtkClipOrigin);
    mapDcmVector2Vtk(dcmClipNormal, vtkClipNormal);
    // Set the clip plane origin and normal...
    dicomInfo.vtk_clipPlane->SetOrigin(vtkClipOrigin);
    dicomInfo.vtk_clipPlane->SetNormal(vtkClipNormal);
    // Update the clip depth text information...
    QString depthString;
    depthString.sprintf("Clip Depth (%6.1f)", clipInfo.depth);
    ui->clipDepthLabel->setText(depthString);
    ui->clipDepthLabel->update();
    //
    if (clipInfo.active) {
        displayChanged = true;
    }
}

// Enable or disable clipping...
void augReality::setClipPlaneActive(bool clipActive)
{
    // Set the clipping state properly...
    if (clipInfo.active == clipActive) return;
    clipInfo.active = clipActive;
    if (clipActive) {
        dicomInfo.vtk_mapper->AddClippingPlane(dicomInfo.vtk_clipPlane);
    }
    else {
        dicomInfo.vtk_mapper->RemoveAllClippingPlanes();
    }
    displayChanged = true;
}

// Set current sensor position...
void augReality::setSensorPos(int sensorNum, sensorPos *newPos)
{
    double *pos = newPos->pos;
    double *norm = newPos->norm;
    double normScale = 15.0;
    double vtkPos[3], vtkNorm[3];
    // Ignore bogus calls...
    if (sensorNum < 0 || sensorNum >= MAX_SENSOR)
        return;
    // Save the sensor position, making sure the normal is a unit vector......
    double nLen = sqrt(pow(norm[0], 2.0) + pow(norm[1], 2.0) + pow(norm[2], 2.0));
    for (int i = 0; i < 3; i++) {
        sensorInfo[sensorNum].sourcePos[i] = pos[i];
        sensorInfo[sensorNum].norm[i] = norm[i] / nLen;
    }
    // Map the position and normal vector from DICOM into VTK space...
    matMultPt(dicomInfo.dcm2vtk, sensorInfo[sensorNum].sourcePos, vtkPos);
    mapDcmVector2Vtk(sensorInfo[sensorNum].norm, vtkNorm);
    // Display a green sphere at the Sensor position...
    sensorInfo[sensorNum].vtk_posMarker->SetCenter(vtkPos);
    // Display a green cylinder from the Sensor position in the direction of the normal vector...
    sensorInfo[sensorNum].vtk_normLine->SetPoint1(vtkPos);
    sensorInfo[sensorNum].vtk_normLine->SetPoint2(vtkPos[0] + vtkNorm[0] * normScale,
            vtkPos[1] + vtkNorm[1] * normScale,
            vtkPos[2] + vtkNorm[2] * normScale);
    // If we're 'locked' to the sensor
    // Then change the clip and target information as well...
    if (ui->sensorLock->isChecked() && sensorNum == 0) {
        setClipPlaneSource(pos);
        setTargetSourcePos(pos);
    }
    if (sensorInfo[sensorNum].visible) {
        displayChanged = true;
    }
}

// Set current sensor position...
void augReality::setSensorPos(int sensorNum, double *pos, double *norm)
{
    double normScale = 15.0;
    double vtkPos[3], vtkNorm[3];
    // Ignore bogus calls...
    if (sensorNum < 0 || sensorNum >= MAX_SENSOR)
        return;
    // Save the sensor position, making sure the normal is a unit vector......
    double nLen = sqrt(pow(norm[0], 2.0) + pow(norm[1], 2.0) + pow(norm[2], 2.0));
    for (int i = 0; i < 3; i++) {
        sensorInfo[sensorNum].sourcePos[i] = pos[i];
        sensorInfo[sensorNum].norm[i] = norm[i] / nLen;
    }
    // Map the position and normal vector from DICOM into VTK space...
    matMultPt(dicomInfo.dcm2vtk, sensorInfo[sensorNum].sourcePos, vtkPos);
    mapDcmVector2Vtk(sensorInfo[sensorNum].norm, vtkNorm);
    // Display a green sphere at the Sensor position...
    sensorInfo[sensorNum].vtk_posMarker->SetCenter(vtkPos);
    // Display a green cylinder from the Sensor position in the direction of the normal vector...
    sensorInfo[sensorNum].vtk_normLine->SetPoint1(vtkPos);
    sensorInfo[sensorNum].vtk_normLine->SetPoint2(vtkPos[0] + vtkNorm[0] * normScale,
            vtkPos[1] + vtkNorm[1] * normScale,
            vtkPos[2] + vtkNorm[2] * normScale);
    // If we're 'locked' to the sensor
    // Then change the clip and target information as well...
    if (ui->sensorLock->isChecked() && sensorNum == 0) {
        setClipPlaneSource(pos);
        setTargetSourcePos(pos);
    }
    if (sensorInfo[sensorNum].visible) {
        displayChanged = true;
    }
}

// Set the visibility of a sensor...
void augReality::setSensorVisibility(int sensorNum, bool visible)
{
    if (sensorNum > MAX_SENSOR) return;
    if (visible == sensorInfo[sensorNum].visible) return;
    sensorInfo[sensorNum].visible = visible;
    sensorInfo[sensorNum].vtk_posActor->SetVisibility(visible);
    sensorInfo[sensorNum].vtk_normActor->SetVisibility(visible);
    displayChanged = true;
}

// Set the position from which we are 'approaching' the target position...
void augReality::setTargetSourcePos(double *sourcePos)
{
    double vtkSrcePos[3];
    // Save the source position...
    for (int i = 0; i < 3; i++) {
        targetInfo.sourcePos[i] = sourcePos[i];
    }
    // Map the source position from DICOM into VTK space...
    matMultPt(dicomInfo.dcm2vtk, sourcePos, vtkSrcePos);
    // Update the trajectory to start at the new source position...
    targetInfo.vtk_trajectoryLine->SetPoint1(vtkSrcePos);
    if (targetInfo.trajectoryVisible) {
        displayChanged = true;
    }
}

// Set the target position...
void augReality::setTargetDestPos(double *destPos)
{
    double vtkDestPos[3];
    // Save the destination position...
    for (int i = 0; i < 3; i++) {
        targetInfo.destPos[i] = destPos[i];
    }
    // Map the target position from DICOM into VTK space...
    matMultPt(dicomInfo.dcm2vtk, destPos, vtkDestPos);
    // Move the target sphere to the new target position...
    targetInfo.vtk_targetMarker->SetCenter(vtkDestPos);
    // Update the trajectory to end at the new target position...
    targetInfo.vtk_trajectoryLine->SetPoint2(vtkDestPos);
    if (targetInfo.trajectoryVisible || targetInfo.targetVisible) {
        displayChanged = true;
    }
}

// Set the visibility of the target...
void augReality::setTargetVisibility(bool targetVisible)
{
    if (targetInfo.targetVisible == targetVisible) return;
    targetInfo.targetVisible = targetVisible;
    targetInfo.vtk_targetActor->SetVisibility(targetVisible);
    displayChanged = true;
}

// Set the visibility of the trajectory...
void augReality::setTrajectoryVisibility(bool trajectoryVisible)
{
    if (targetInfo.trajectoryVisible == trajectoryVisible) return;
    targetInfo.trajectoryVisible = trajectoryVisible;
    targetInfo.vtk_trajectoryActor->SetVisibility(trajectoryVisible);
    displayChanged = true;
}

// Update the display if anything changed...
void augReality::updateDisplay()
{
    if (!displayChanged) return;
    vtk_renderWindow->Render();
    displayChanged = false;
}

void augReality::on_displayVolume_clicked(bool checked)
{
    setDicomVisibility(checked);
    updateDisplay();
}

void augReality::on_displayTarget_clicked(bool checked)
{
    setTargetVisibility(checked);
    updateDisplay();
}

void augReality::on_displayTrajectory_clicked(bool checked)
{
    setTrajectoryVisibility(checked);
    updateDisplay();
}

void augReality::on_displayClipping_clicked(bool checked)
{
    setClipPlaneActive(checked);
    updateDisplay();
}

void augReality::on_displaySensor_clicked(bool checked)
{
    for (int i=0; i < MAX_SENSOR; i++) {
        setSensorVisibility(i, checked);
    }
    updateDisplay();
}

void augReality::on_trackingIP_currentIndexChanged(const QString &arg1)
{
    std::cout << "augreality: Yo! -" << arg1.toStdString() << "-\n";
}

void augReality::on_sensorLock_clicked(bool checked)
{
    if (checked)
        std::cout << "augreality: Locked\n";
    else
        std::cout << "augreality: Free\n";
}

void augReality::calcClipAngle(double *theta, double *phi)
{
    double cdPt[3], csPt[3], cVect[3], cost;
    findProjection(cdPt, clipInfo.destPos, dicomInfo.raiPos, dicomInfo.xDir);
    findProjection(csPt, clipInfo.sourcePos, dicomInfo.raiPos, dicomInfo.xDir);
    for (int i = 0; i < 3; i++) cVect[i] = -csPt[i] + cdPt[i];
    cost = dot3(cVect, dicomInfo.zDir) / sqrt(dot3(cVect, cVect));
    *theta = acos(cost) * 360.0 / 6.2831853;
    *phi = 0;

    double dot = cVect[0]*dicomInfo.zDir[0] + cVect[1]*dicomInfo.zDir[1] + cVect[2]*dicomInfo.zDir[2];
    double det = cVect[0]*dicomInfo.zDir[1]*dicomInfo.xDir[2] + dicomInfo.zDir[0]*dicomInfo.xDir[1]*cVect[2] + dicomInfo.xDir[0]*cVect[1]*dicomInfo.zDir[2]
            - cVect[2]*dicomInfo.zDir[1]*dicomInfo.xDir[0] - dicomInfo.zDir[2]*dicomInfo.xDir[1]*cVect[0] - dicomInfo.xDir[2]*cVect[1]*dicomInfo.zDir[0];
    *phi = atan2(det, dot) * 360.0 / 6.2831853;
}

// Find the perpendicular projection of a point onto the plane defined
//      by a point on the plane and the plane normal vector...
void augReality::findProjection(double *projPt, double *linePt, double *planePt, double *planeNorm)
{
    double planePt_m_linePt[3], dist;
    for (int i = 0; i < 3; i++) {
        planePt_m_linePt[i] = planePt[i] - linePt[i];
    }
    dist = dot3(planePt_m_linePt, planeNorm) / dot3(planeNorm, planeNorm);
    for (int i = 0; i < 3; i++) {
        projPt[i] = dist * planeNorm[i] + linePt[i];
    }
}

// Dot product of two vectors...
double augReality::dot3(double *v1, double *v2)
{
    double dProd = 0;
    for (int i = 0; i < 3; i++) {
        dProd += v1[i] * v2[i];
    }
    return(dProd);
}

// Cross product of two vectors...
void augReality::cross3(double *cProd, double *v1, double *v2)
{
    cProd[0] =  (v1[1]*v2[2] - v2[1]*v1[2]);
    cProd[1] = -(v1[0]*v2[2] - v2[0]*v1[2]);
    cProd[2] =  (v1[0]*v2[1] - v2[0]*v1[1]);
}

void augReality::print3(std::string prompt, double *array)
{
    std::cout << prompt << "\n";
    for (int i = 0; i < 3; i++)
        std::cout << "  " << array[i];
    std::cout << "\n";
}

void augReality::on_polarAngleSlider_valueChanged(int value)
{
    int phiSlider;
    double theta, phi;
    theta = (2.0 * PI / 359.0) * (double)value - PI;
    phiSlider = ui->azimuthAngleSlider->value();
    phi = (2.0 * PI / 359.0) * (double)phiSlider - PI;
    setSensorPos(0, theta, phi);
    // Update the display...
    updateDisplay();
}

void augReality::on_azimuthAngleSlider_valueChanged(int value)
{
    int thetaSlider;
    double theta, phi;
    phi = (2.0 * PI / 359.0) * (double)value - PI;
    thetaSlider = ui->polarAngleSlider->value();
    theta = (2.0 * PI / 359.0) * (double)thetaSlider - PI;
    setSensorPos(0, theta, phi);
    // Update the display...
    updateDisplay();
}

void augReality::setSensorPos(int sensorNum, double theta, double phi)
{
    double vtkDelta[3], dcmDelta[3], dcmPos[3];
    convertSphericalToCartesian(vtkDelta, theta, phi, dicomInfo.xFOV / 2.0);
    matMultPt(dicomInfo.vtk2dcm, vtkDelta, dcmDelta);
    for (int i = 0; i < 3; i++) {
        dcmPos[i] = dcmDelta[i] - dicomInfo.raiPos[i] + clipInfo.destPos[i];
    }
    setSensorPos(sensorNum, dcmPos, dicomInfo.zDir);
    // Update the angular text information...
    QString infoString;
    infoString.sprintf("Polar Angle (%6.1f)", theta * 180.0 / PI);
    ui->polarLabel->setText(infoString);
    ui->polarLabel->update();

    infoString.sprintf("Azimuthal Angle (%6.1f)", phi * 180.0 / PI);
    ui->azimuthLabel->setText(infoString);
    ui->azimuthLabel->update();
}

void augReality::on_quitButton_clicked()
{
    std::cout << "augreality: quit?\n";
}

// Create a symbolic link to a file in the specified target directory...
std::string augReality::createSymlink(std::string targetDir, std::string fullFn)
{
    std::string newPath;
    int lastSlash = fullFn.find_last_of("/");
    newPath = targetDir + "/" + fullFn.substr(lastSlash+1);
    int res = symlink(fullFn.c_str(), newPath.c_str());
    if (res) {
        std::cout << "augreality: symlink failure " << errno << "\n";
    }
    return(newPath);
}

void augReality::convertSphericalToCartesian(double *cartCoord, double theta, double phi, double r)
{
#if 1
    // Note that 'Y' and 'Z' are interchanged from the 'standard' spherical coordinate calculations...
    cartCoord[0] = (r * sin(theta) * cos(phi));
    cartCoord[2] = (r * sin(theta) * sin(phi));
    cartCoord[1] = (r * cos(theta));
#else
// Rx...
//    cartCoord[0] = 0;
//    cartCoord[1] = r * cos(theta);
//    cartCoord[2] = r * sin(theta);
// Rz...
//    cartCoord[0] = -r * sin(phi);
//    cartCoord[1] = r * cos(phi);
//    cartCoord[2] = 0;
// Rx * Rz...
    cartCoord[0] = -r * sin(phi);
    cartCoord[1] = r * cos(phi) * cos(theta);
    cartCoord[2] = r * cos(phi) * sin(theta);
#endif
}


void augReality::on_resetClip_clicked()
{
    std::cout << "augreality: Reset\n";
    setClipPlaneDepth(0);
    setSensorPos(0, 0.0, 0.0);
    // Update the display...
    updateDisplay();
}

void augReality::on_trackingCheckBox_clicked(bool checked)
{
    dicomVolume.load_dcmDirectory("/media/data_gauss/augmentedReality/rao_11052015/DICOM_T1");
    dicomVolume.dbDump();
    loadDicomDirectory("/media/data_gauss/augmentedReality/rao_11052015/DICOM_T1");
    updateDisplay();
//    dbDump();
}

void augReality::on_actionQuit_triggered()
{
    std::cout << "augreality: Quit!\n";
}

void augReality::on_actionOpen_2_triggered()
{
    std::cout << "augreality: open\n";
    std::string studyUID, seriesUID;
    //
    // Allow the user to select a DICOM series...
    int result = dcmCat.pickStudy(&studyUID, &seriesUID);
    std::cout << "augreality:   - returned " << result << "\n";
    std::cout << "  - studyUID = " << studyUID << "\n";
    std::cout << "  - seriesUID = " << seriesUID << "\n";
    if (result == 0) return;
    QStringList imageList = dcmCat.getImageList(studyUID, seriesUID);
    dicomVolume.load_dcmFiles(imageList);


    std::string imgFname;
    std::string symLink;
    QStringList symLinkList;
    // Create links to the series' image files in our temporary directory...
    for (int sei = 0; sei < imageList.size(); ++sei) {
        imgFname = imageList.at(sei).toLocal8Bit().constData();
        symLink = createSymlink(dicomTempDir, imgFname);
        symLinkList << symLink.c_str();
    }
    // Load and display the series...
    loadDicomDirectory(dicomTempDir);
    updateDisplay();
    // Then remove the links to the image files in our temporary directory...
    for (int sei = 0; sei < symLinkList.size(); ++sei) {
        imgFname = symLinkList.at(sei).toLocal8Bit().constData();
        int res = unlink(imgFname.c_str());
        if (res) {
            std::cout << "  Error " << errno << " unlinking " << imgFname << "\n";
        }
    }
\

}

void augReality::on_actionCatalog_directory_triggered()
{
    std::cout << "augreality: catalog\n";

}

void augReality::on_endoConnect_clicked()
{
    int conStatus = p_endoQuery->getConnectStatus();
    std::cout << "augreality: Connection status: " << conStatus << "\n";
    if (conStatus == ECS_IDLE) {
        p_endoQuery->endoConnect("192.168.2.5", 20248);
    }
    else if (conStatus == ECS_CONNECTED) {
        p_endoQuery->endoDisconnect();
    }
}
