#ifndef AUGREALITY_H
#define AUGREALITY_H

#include <QMainWindow>
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPlane.h"
#include "vtkLineSource.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkDICOMImageReader.h"
#include "vtkColorTransferFunction.h"
#include "dcmCatalog.h"
#include "vtkMatrix4x4.h"
#include "vtkVolumeTextureMapper2D.h"
#include "QModelIndex"
#include "QComboBox"
#include "endoQuery.h"
#include "vtkdicom.h"

#define MAX_SENSOR  2
// Clip plane data...
struct clipData {
    bool active;
    bool changed;
    double sourcePos[3];
    double destPos[3];
    double depth;
};

// DICOM data...
struct dicomData {
    bool visible;
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
    // Transform matrices to map between DICOM and VTK coordinates...
    //  NOTE: xf_pt = XF_MAT * pt
    vtkSmartPointer<vtkMatrix4x4> dcm2vtk, vtk2dcm;
    vtkSmartPointer<vtkVolume> vtk_volume;
    vtkSmartPointer<vtkVolumeTextureMapper2D> vtk_mapper;
    vtkSmartPointer<vtkDICOMImageReader> vtk_reader;
    vtkSmartPointer<vtkColorTransferFunction> vtk_lut;
    vtkSmartPointer<vtkPlane> vtk_clipPlane;
};

// Sensor data (in DICOM coordinates)...
struct sensorData {
    bool visible;
    double sourcePos[3], norm[3];
    vtkSmartPointer<vtkSphereSource> vtk_posMarker;
    vtkSmartPointer<vtkActor> vtk_posActor;
    vtkSmartPointer<vtkLineSource> vtk_normLine;
    vtkSmartPointer<vtkActor> vtk_normActor;
};

// Target and trajectory (=source to target) data...
struct targetData {
    bool targetVisible, trajectoryVisible;
    double sourcePos[3], destPos[3];
    vtkSmartPointer<vtkSphereSource> vtk_targetMarker;
    vtkSmartPointer<vtkActor> vtk_targetActor;
    vtkSmartPointer<vtkLineSource> vtk_trajectoryLine;
    vtkSmartPointer<vtkActor> vtk_trajectoryActor;
};

namespace Ui {
class augReality;
}

class augReality : public QMainWindow
{
    Q_OBJECT

public:
    explicit augReality(QWidget *parent = 0);
    ~augReality();
    void imgUpdate();

    
private slots:
    void on_quitButton_clicked();
    void on_depthScrollBar_valueChanged(int value);
    void on_displayVolume_clicked(bool checked);
    void on_displayTarget_clicked(bool checked);
    void on_displayTrajectory_clicked(bool checked);
    void on_displayClipping_clicked(bool checked);
    void on_displaySensor_clicked(bool checked);
    void on_trackingIP_currentIndexChanged(const QString &arg1);
    void on_sensorLock_clicked(bool checked);
    void on_polarAngleSlider_valueChanged(int value);
    void on_azimuthAngleSlider_valueChanged(int value);
    void on_resetClip_clicked();
    void on_trackingCheckBox_clicked(bool checked);
    void on_actionQuit_triggered();
    void on_actionOpen_2_triggered();
    void on_actionCatalog_directory_triggered();
    void on_endoConnect_clicked();

private:
    Ui::augReality *ui;
    vtkSmartPointer<vtkRenderWindow> vtk_renderWindow;
    vtkSmartPointer<vtkRenderer> vtk_renderer;
    dcmCatalog dcmCat;
    bool endoConnected;
    bool activeTracking;
    int numSensors;
    bool displayChanged;
    dicomData dicomInfo;
    sensorData sensorInfo[MAX_SENSOR];
    targetData targetInfo;
    clipData clipInfo;
    int test;
    endoQuery *p_endoQuery;
    int currEndoStatus;
    std::string dicomTempDir;
    vtkDicom dicomVolume;
    //
    void initVars();
    void initVtk();
    void dumpDicomReader(vtkSmartPointer<vtkDICOMImageReader>);
    void matMultPt(vtkSmartPointer<vtkMatrix4x4>, double *inp3, double *outp3);
    void mapDcmVector2Vtk(double *dcmVect , double *vtkVect);
    void loadDicomDirectory(std::string);
    void loadDicomInfo();
    void setDicomVisibility(bool);
    std::string createSymlink(std::string dir, std::string fn);

    void setClipPlane(double *source, double *dest, double depth);
    void setClipPlaneSource(double *);
    void setClipPlaneDest(double *);
    void setClipPlaneDepth(double);
    void updateClipPlane();
    void setClipPlaneActive(bool);

    void dbDump();
    int pickStudy(std::string *studyUID, std::string *seriesUID);

    void setSensorPos(int sensorNum, double *pos, double *norm);
    void setSensorPos(int sensorNum, double theta, double phi);
    void setSensorPos(int sensorNum, sensorPos *newPos);
    void setSensorVisibility(int sensorNum, bool visibility);
    void setTargetSourcePos(double *);
    void setTargetDestPos(double *);
    void setTargetVisibility(bool);
    void setTrajectoryVisibility(bool);
    void updateDisplay();

    void findProjection(double *projPt, double *inPt, double *planePt, double *planeNVect);
    void convertSphericalToCartesian(double *cartCoord, double theta, double phi, double r);
    void calcClipAngle(double *theta, double *phi);
    double dot3(double *v1, double *v2);
    void cross3(double *cProd, double *v1, double *v2);
    void print3(std::string prompt, double *array);

};

// TIMER***********************************
#include <QTimer>
class MyTimer : public QObject
{
    Q_OBJECT
public:
  QTimer *timer;
  augReality *l_this;
  void start(augReality *p_this) {
    l_this = p_this;
    // msec
    timer->start(100);
  }
  MyTimer() {
    // create a timer
    timer = new QTimer(this);
    // setup signal and slot
    connect(timer, SIGNAL(timeout()), this, SLOT(MyTimerSlot()));
  }
public slots:
  void MyTimerSlot() {
    l_this->imgUpdate();
  }
};
// TIMER***********************************

#endif // AUGREALITY_H
