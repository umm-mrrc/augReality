#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "unistd.h"

#include "endoQuery.h"

#define ENDO_VECT_REQUEST "DATA 1"
#define ENDO_BAD_REQUEST "NADA_NADA"
#define ENDO_QUATERNION_REQUEST "\r"

//********************************************************
//********************************************************
//  EndoScout Query Thread routines...

// Query the Endoscout and set the raw position string...
int et_getEndoPos(threadData *p_threadData)
{
    int port;
    float xyz[3];
    float nvt[3];
    float tvt[3];
    int statusCode;
    float cf, rr;
    int wc;
    ssize_t nBytes;
    if (p_threadData->endoSock == -1) return(0);
    // Send a query request to the endoscout...
    nBytes = send(p_threadData->endoSock, ENDO_VECT_REQUEST, strlen(ENDO_VECT_REQUEST), 0);
    if (nBytes == -1) {
        std::cout << "endoQuery - getRawEndoPos send error: " << errno << "\n";
        return(0);
    }
    // Read the response from the endoscout...
    memset(p_threadData->endoBuffer, 0, 1024);
    int bytesRcvd = recv(p_threadData->endoSock, p_threadData->endoBuffer, 1024, 0);
    if (bytesRcvd == -1) {
        std::cout << "endoQuery - getRawEndoPos Recv error: " << errno << "\n";
        return(0);
    }
    // Parse the response...
    char *token = strtok(p_threadData->endoBuffer, "\n");
    pthread_mutex_lock(&(p_threadData->lock));
    while (token) {
        sscanf(token, "DATA%d %f %f %f %f %f %f %f %f %f %d %f %f %d\n",
               &port,
               &xyz[0], &xyz[1], &xyz[2],
                &nvt[0], &nvt[1], &nvt[2],
                &tvt[0], &tvt[1], &tvt[2], &statusCode, &cf, &rr,
                &wc);
        if (port > 0 && port < MAX_ENDO) {
            p_threadData->currPos[port].status = statusCode;
            p_threadData->currPos[port].cf = cf;
            p_threadData->currPos[port].warn = wc;
            for (int nc = 0; nc < 3; nc++) {
                p_threadData->currPos[port].pos[nc] = xyz[nc];
                p_threadData->currPos[port].norm[nc] = nvt[nc];
                p_threadData->currPos[port].trans[nc] = tvt[nc];
            }
        }
        token = strtok(NULL, "\n");
    };
    pthread_mutex_unlock(&(p_threadData->lock));
    return(1);
}

// Attempt to open a TCP connection to the EndoScout
//   Returns 0|-1.. = Connected|Not connected
int et_connect(threadData *p_threadData, queuedRequest *p_connRq) {
    char buffer[512];
    struct sockaddr_in servAddr;
    // Create a TCP socket...
    if (p_threadData->verbose) std::cout << "endoQuery: creating socket...\n";
    p_threadData->endoSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (p_threadData->endoSock < 0) {
        if (p_threadData->verbose) std::cout << "endoQuery: ERROR creating TCP socket\n";
        return(-1);
    }
    // Set up to timeout after 5 seconds on a 'recv'...
    struct timeval send_timeout = {5, 0};
    setsockopt (p_threadData->endoSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&send_timeout, sizeof (struct timeval));
    // Construct the server address structure...
    bzero((char *) &servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(p_connRq->rqData.crq.ip);
    servAddr.sin_port = htons(p_connRq->rqData.crq.port);
    // Attempt to connect to the endoscout...
    if (p_threadData->verbose) {
        std::cout << "endoQuery: attempting to connect to " << p_connRq->rqData.crq.ip << ":" << p_connRq->rqData.crq.port << "\n";
    }
    if (connect(p_threadData->endoSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
        std::cout << "endoQuery: ERROR connecting to Endoscout (" << strerror(errno) << ")\n";
        close(p_threadData->endoSock);
        p_threadData->endoSock = -1;
        return(-2);
    }
    // Send a request and see if we get the response we're expecting...
    if (p_threadData->verbose) std::cout << "endoQuery: query and check response...\n";
    send(p_threadData->endoSock, ENDO_VECT_REQUEST, strlen(ENDO_VECT_REQUEST), 0);
    bzero(buffer, sizeof(buffer));
    int bytesRcvd = recv(p_threadData->endoSock, buffer, 512, 0);
    if (bytesRcvd == -1) {
        std::cout << "endoQuery et_connect - Initial Endoscout response timeout.\n";
        close(p_threadData->endoSock);
        p_threadData->endoSock = -1;
        return(-3);
    }
    if (strncmp("#Server Ready", buffer, 13) != 0) {
        std::cout << "endoQuery: Invalid initial response from Endoscout:\n";
        std::cout << "   ???:" << buffer << std::endl;
        close(p_threadData->endoSock);
        p_threadData->endoSock = -1;
        return(-4);
    }
    // Success!
    if (p_threadData->verbose) std::cout << "endoQuery: Endoscout connected and responding.\n";
    strcpy(p_threadData->endoIP, p_connRq->rqData.crq.ip);
    p_threadData->endoPort = p_connRq->rqData.crq.port;
    return(0);
}

void et_disconnect(threadData *p_threadData)
{
    if (p_threadData->endoSock != -1) {
        close(p_threadData->endoSock);
        p_threadData->endoSock = -1;
        if (p_threadData->verbose) std::cout << "endoQuery: Endoscout disconnected.\n";
    }
}

// Function started as a thread to query the endoscout and post the results...
void *et_mainLoop(void *p_threadData)
{
    int result;
    threadData *lp_threadData = (threadData *)p_threadData;
    struct timespec *lDelay = &(lp_threadData->endoDelay);
    // Do forever...
    for (;;) {
        // Process any queued requests...
        pthread_mutex_lock(&(lp_threadData->lock));
        queuedRequest *p_nextRq = lp_threadData->requestQueue;
        lp_threadData->requestQueue = (p_nextRq)?p_nextRq->p_next:NULL;
        pthread_mutex_unlock(&(lp_threadData->lock));
        if (p_nextRq) {
            switch (p_nextRq->requestCode)  {
            case ETQR_CONNECT:
                lp_threadData->endoConnStatus = ECS_CONNECTING;
                result = et_connect(lp_threadData, p_nextRq);
                lp_threadData->endoConnStatus = (result == 0)?ECS_CONNECTED:ECS_IDLE;
                break;
            case ETQR_DISCONNECT:
                et_disconnect(lp_threadData);
                lp_threadData->endoConnStatus = ECS_IDLE;
                break;
            }
            delete p_nextRq;
        }
        // If we're connected
        // Then attempt to read the EndoScout...
        if (lp_threadData->endoConnStatus == ECS_CONNECTED) {
            int result = et_getEndoPos(lp_threadData);
        }
        // Sleep for a bit before continuing...
        if (nanosleep(lDelay, NULL)) {};
    }
}

//  ...EndoScout Query Thread routines
//********************************************************
//********************************************************

// Destructor to close any communications nicely...
endoQuery::~endoQuery()
{
    endoDisconnect();
    delete(p_threadData);
}

// Default constructor to initalize some things...
endoQuery::endoQuery()
{
    float endoThreadDelay = .1;
    std::cout << "endoQuery: initialization\n";
    // Initialize our variables...
    p_threadData = new threadData;
    p_threadData->endoConnStatus = ECS_IDLE;
    p_threadData->endoIP[0] = 0;
    p_threadData->endoPort = -1;
    p_threadData->endoDelay.tv_sec = (int)endoThreadDelay;
    p_threadData->endoDelay.tv_nsec = 1e9 * (endoThreadDelay - (float)p_threadData->endoDelay.tv_sec);
    pthread_mutex_init(&p_threadData->lock, NULL);
    p_threadData->endoSock = -1;
    p_threadData->verbose = 1;
    p_threadData->requestQueue = NULL;
    memset(p_threadData->endoBuffer, 0, 1024);
    for (int nSensor = 0; nSensor < MAX_ENDO; nSensor++) {
        p_threadData->currPos[nSensor].status = 8000;
        p_threadData->currPos[nSensor].cf = -1;
        p_threadData->currPos[nSensor].warn = -1;
        for (int nc = 0; nc < 3; nc++) {
            p_threadData->currPos[nSensor].pos[nc] = -1;
            p_threadData->currPos[nSensor].norm[nc] = -1;
            p_threadData->currPos[nSensor].trans[nc] = -1;
        }
    }
    // Start the EndoQuery thread...
    int iReturnValue = pthread_create(&eqThreadId, NULL, &et_mainLoop, (void *)p_threadData);
    if (iReturnValue) {
        std::cout << "endoQuery: Unable to create query thread. Aborting!\n";
        exit(0);
    }
}

void endoQuery::queueRequest(queuedRequest *newRq)
{
    // Queue the request for the thread...
    newRq->p_next = NULL;
    pthread_mutex_lock(&(p_threadData->lock));
    queuedRequest *currRq = p_threadData->requestQueue;
    if (currRq == NULL) {
        p_threadData->requestQueue = newRq;
    }
    else {
        while (currRq->p_next != NULL) currRq = currRq->p_next;
        currRq->p_next = newRq;
    }
    pthread_mutex_unlock(&(p_threadData->lock));
}

// Queue a request to the EndoQuery thread to connect to the Endoscout...
void endoQuery::endoConnect(const char *endoIP, const int endoPort)
{
    queuedRequest *connRq = new queuedRequest;
    connRq->requestCode = ETQR_CONNECT;
    strcpy(connRq->rqData.crq.ip, endoIP);
    connRq->rqData.crq.port = endoPort;
    queueRequest(connRq);
}

// Disconnect from the Endoscout if we're connected...
void endoQuery::endoDisconnect()
{
    queuedRequest *dconnRq = new queuedRequest;
    dconnRq->requestCode = ETQR_DISCONNECT;
    queueRequest(dconnRq);
}

// Set verbosity...
void endoQuery::setVerbose(int lverbose)
{
    p_threadData->verbose = lverbose;
}

int endoQuery::getConnectStatus()
{
    pthread_mutex_lock(&(p_threadData->lock));
    int connStatus = p_threadData->endoConnStatus;
    pthread_mutex_unlock(&(p_threadData->lock));
    return(connStatus);
}

int endoQuery::getEndoPos(int sensorNum, sensorPos *sensorData)
{
    if (sensorNum < 0 || sensorNum > MAX_ENDO) {
        return(-1);
    }
    pthread_mutex_lock(&(p_threadData->lock));
    memcpy(sensorData, &p_threadData->currPos[sensorNum], sizeof(sensorPos));
    pthread_mutex_unlock(&(p_threadData->lock));
    return(0);
}
