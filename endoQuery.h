/*****************************************
File: endoQuery.h
Author: Steve Roys
        3/8/2013
        1/12/2016 Modified for Robin Medical augmented reality project

This module implements the class used to interface with a Robin Medical
Endoscout and provide position information.  It will connect to the 
Endoscout on a given IP address and port, and will query and return
the position string from the EndoScout when requested.
*****************************************/
#ifndef ENDOQUERY_H
#define ENDOQUERY_H

#include <stdint.h>

#define MAX_ENDO 3

// EndoScout Connection status codes:
#define ECS_IDLE        0       // IDLE/not connected
#define ECS_RQ_CONN     1       // Connection Requested
#define ECS_CONNECTING  2       // Attempting to connect
#define ECS_CONNECTED   3       // Connected

// EndoScout thread queued request codes:
#define ETQR_CONNECT    0       // Connect to EndoScout
#define ETQR_DISCONNECT 1       // Disconnect from EndoScout

struct connRequest {
    char ip[128];               // IP address
    int port;                   // Port number
};

struct queuedRequest {          // EndoScout thread queued request:
    queuedRequest *p_next;      //   Next queued request
    int requestCode;            //   Request code
    union {                     //   Request data:
        struct connRequest crq; //     Connection request data
    } rqData;
};

struct sensorPos {              // Parsed EndoScout position data:
    int status;                 //   Status (1000|8000 = good|poor tracking)
    float cf;                   //   Cost function
    int warn;                   //   Warning code (0=OK)
    double pos[3];              //   Position = PX, PY, PZ
    double norm[3];             //   Norm vector = NX, NY, NZ
    double trans[3];            //   Transverse vector = TX, TY, TZ
};

struct threadData {              // EndoScout thread accessible data:
    char endoIP[128];            //   IP address of Endoscout
    int endoPort;                //   TCP port of Endoscout
    struct timespec endoDelay;   //   Nanosleep time for endoscout query thread
    pthread_mutex_t lock;        //   Exclusion lock for threads
    int endoSock;                //   Socket for connection to endoscout
    int verbose;                 //   Verbose flag for debugging
    int endoConnStatus;          //   EndoScout connection status
    queuedRequest *requestQueue; //   EndoScout thread request queue
    char endoBuffer[1024];       //   'Raw' EndoScout position data
    sensorPos currPos[MAX_ENDO]; //   Parsed EndoScout position data
};

// EndoScout Thread routines...
int et_getEndoPos(threadData *p_threadData);
int et_connect(threadData *p_threadData, queuedRequest *p_connRq);
void et_disconnect(threadData *p_threadData);
void *et_mainLoop(void *p_threadData);

class endoQuery {
    // Private:
    threadData *p_threadData;  // EndoScout thread accessible data
    pthread_t eqThreadId;      // EndoScout thread ID
    void queueRequest(queuedRequest *newRq);
public:
    // Public methods:
    ~endoQuery();
    endoQuery();
    void endoConnect(const char *ipAddress, const int ipPort);
    void endoDisconnect();
    int getConnectStatus();
    void setVerbose(int verbose);
    void setEndoQueryDelay(float delay);
    int setEndoQueryActive(bool enabled);
    int getEndoPos(int sensorNum, sensorPos *sensorData);
};
#endif
