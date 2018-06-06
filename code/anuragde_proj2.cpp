/*
 * RoutingProtocolProj2.cpp
 *
 *  Created on: Nov 4, 2016
 *      Author:n anurag

 References:
 http://stackoverflow.com/questions/2086126/need-programs-that-illustrate-use-of-settimer-and-alarm-functions-in-gnu-c

 */
#include "iostream"
#include "string"
#include "cstring"
#include <stdlib.h>
#include <unistd.h>
#include "cstdlib"
#include <errno.h>
#include "fstream"
#include "sstream"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#define STDIN_DESC 0

using namespace std;;
int updateInterval;
string task1,sub;
fd_set master,readfds;
int fd,sockfd,maxfd;
int listener_socket;
int recvData=0,recvDataTotal=0;
struct server{
	int serverId;
	char ip[16];
	int port;
};

struct routingTable{
	int numServers;
	int numNeighbors;
	int selfId,neighborNode[5];
	int Cost[6][6];
	int minCost[6][6];
	/*routing table format*/
	int nextHop[6][6];
	string status;
};
server serverList[6];//Contains the details of all the servers
routingTable selfRoutingTable;
routingTable neighbourRoutingTable;
char *x;
struct sockaddr_in nbaddr,sab;
int counter1[]={1,1,1,1,1};
int getIndex(int a){
	for(int i=0;i<selfRoutingTable.numNeighbors;i++){
		if(selfRoutingTable.neighborNode[i]==a){
			//cout<<selfRoutingTable.numNeighbors<<" getindex "<<selfRoutingTable.neighborNode[i]<<" "<<i<<" "<<a<<endl;
			return i;}
	}
	//cerr<<"Link doesn't exist";
	int x=-1000;
	return x;
}

void recomputeCostMatrix(int a,int b,int c){
	selfRoutingTable.Cost[a][b]=c;
	selfRoutingTable.Cost[b][a]=c;
	selfRoutingTable.minCost[a][b]=c;
	selfRoutingTable.minCost[b][a]=c;
	int i=1,j=1,k=1;
	if(c==-1000){
		if(selfRoutingTable.nextHop[a][b]==b)
			selfRoutingTable.nextHop[a][b]=-1000;
		for(i=1;i<6;i++){
			if(i==b)
				selfRoutingTable.Cost[b][i]=0;
			else
				selfRoutingTable.Cost[b][i]=-1000;
		}
	}

	if(c!=-1000 && c<selfRoutingTable.minCost[a][b]){
		selfRoutingTable.minCost[a][b]=c;
		selfRoutingTable.nextHop[a][b]=b;

	}

	for(j=1;j<6;j++){
		if(a!=j){
			for(k=1;k<6;k++){
				if(k!=j&&k!=a&&(selfRoutingTable.minCost[a][k]!=-1000)&&(selfRoutingTable.minCost[k][j])!=-1000){
					if(selfRoutingTable.minCost[a][j]==-1000 || (selfRoutingTable.minCost[a][j])>(selfRoutingTable.minCost[a][k]+selfRoutingTable.minCost[k][j]))
					{
						if((selfRoutingTable.nextHop[a][j]==-1000) || selfRoutingTable.nextHop[a][k]==k)
							selfRoutingTable.nextHop[a][j]=k;
						else
							selfRoutingTable.nextHop[a][j]=selfRoutingTable.nextHop[a][k];
						selfRoutingTable.minCost[a][j]=selfRoutingTable.minCost[a][k] + neighbourRoutingTable.minCost[k][j];}
				}
			}
		}
	}
	//cout<<"-Recompute SUCCESS"<<endl;
	return;
}

void incrementCounter(){
	for(int i=0;i<5;i++){
		if(selfRoutingTable.neighborNode[i]!=-1000){
			counter1[i]++;
			//if(i==getIndex(selfRoutingTable.selfId))
				//cout<<"increment counter["<<i<<"] "<<"to "<<counter1[i]<<endl;
			if((counter1[i]==5)&&(selfRoutingTable.selfId != selfRoutingTable.neighborNode[i])){
				recomputeCostMatrix(selfRoutingTable.selfId,selfRoutingTable.neighborNode[i],-1000);
				selfRoutingTable.neighborNode[i]=-1000;
			}
			}
	}
	return;
}
/*Update Message Format*/
struct messageFormat{
	short numUpdateFields;
	short serverPort;
	char serverIP[16];
	char serverIP1[16];
	short serverPort1;
	short buffer1;
	short serverId1;
	char serverIP2[16];
	short serverPort2;
	short buffer2;
	short serverId2;
	char serverIP3[16];
	short serverPort3;
	short buffer3;
	short serverId3;
	char serverIP4[16];
	short serverPort4;
	short buffer4;
	short serverId4;
	char serverIP5[16];
	short serverPort5;
	short buffer5;
	short serverId5;
	short cost[5];
	//int s;
};

messageFormat messagetoSend,messageToReceive;
void createMessage(){
	messagetoSend.numUpdateFields=5;
	messagetoSend.serverPort= serverList[selfRoutingTable.selfId].port;
	strcpy(messagetoSend.serverIP,serverList[selfRoutingTable.selfId].ip);
	strcpy(messagetoSend.serverIP1,serverList[1].ip);
	messagetoSend.serverPort1= serverList[1].port;
	messagetoSend.buffer1;
	messagetoSend.serverId1=serverList[1].serverId;
	messagetoSend.cost[0]=selfRoutingTable.minCost[selfRoutingTable.selfId][messagetoSend.serverId1];
	strcpy(messagetoSend.serverIP2,serverList[2].ip);
	messagetoSend.serverPort2= serverList[2].port;
	messagetoSend.buffer2;
	messagetoSend.serverId2=serverList[2].serverId;
	messagetoSend.cost[1]=selfRoutingTable.minCost[selfRoutingTable.selfId][messagetoSend.serverId2];
	strcpy(messagetoSend.serverIP3,serverList[3].ip);
	messagetoSend.serverPort3= serverList[3].port;
	messagetoSend.buffer3;
		messagetoSend.serverId3=serverList[3].serverId;
		messagetoSend.cost[2]=selfRoutingTable.minCost[selfRoutingTable.selfId][messagetoSend.serverId3];
	strcpy(messagetoSend.serverIP4,serverList[4].ip);
	messagetoSend.serverPort4= serverList[4].port;
	messagetoSend.buffer4;
		messagetoSend.serverId4=serverList[4].serverId;
		messagetoSend.cost[3]=selfRoutingTable.minCost[selfRoutingTable.selfId][messagetoSend.serverId4];
	strcpy(messagetoSend.serverIP5,serverList[5].ip);
	messagetoSend.serverPort5= serverList[5].port;
	messagetoSend.buffer5;
		messagetoSend.serverId5=serverList[5].serverId;
		messagetoSend.cost[4]=selfRoutingTable.minCost[selfRoutingTable.selfId][messagetoSend.serverId5];
}
void stepRoutingTable(){
	//cout<<"Forwarding routing table to neighbors"<<endl;
	createMessage();
	for(int k=0;k<selfRoutingTable.numNeighbors;k++){
		//cout<<selfRoutingTable.numNeighbors<<endl;
		if((selfRoutingTable.minCost[selfRoutingTable.selfId][selfRoutingTable.neighborNode[k]]!=-1000)||(selfRoutingTable.neighborNode[k])==-1000){
			memset((void *)&nbaddr, 0, sizeof(nbaddr));
			nbaddr.sin_family=AF_INET;
			nbaddr.sin_port= htons(serverList[selfRoutingTable.neighborNode[k]].port);
			inet_pton(AF_INET,serverList[selfRoutingTable.neighborNode[k]].ip,&(nbaddr.sin_addr));
			//messagetoSend.s=inet_pton(AF_INET,serverList[selfRoutingTable.selfId].ip,&(sab.sin_addr));
			sendto(listener_socket, &messagetoSend, sizeof(messagetoSend), 0, (struct sockaddr *)&nbaddr, sizeof(nbaddr));
			//if (sendto(listener_socket, &messagetoSend, sizeof(messagetoSend), 0, (struct sockaddr *)&nbaddr, sizeof(nbaddr)) < 0) {

				//if (sendto(listener_socket, &selfRoutingTable, sizeof(selfRoutingTable), 0, (struct sockaddr *)&nbaddr, sizeof(nbaddr)) < 0) {
				//cerr<<"sendto failed"<<endl;
				//	return;
			//}
		}

	}
//	cout<<"-step SUCCESS"<<endl;
	return;
}

void timerMethods(){
	incrementCounter();
	stepRoutingTable();
	return;
}

void stepRoutingTable1(int k){
	createMessage();
	//cout<<"Forwarding routing table to neighbors"<<endl;
	memset((void *)&nbaddr, 0, sizeof(nbaddr));
	nbaddr.sin_family=AF_INET;
	nbaddr.sin_port= htons(serverList[selfRoutingTable.neighborNode[k]].port);
	inet_pton(AF_INET,serverList[selfRoutingTable.neighborNode[k]].ip,&(nbaddr.sin_addr));
	messagetoSend.numUpdateFields=1;
if (sendto(listener_socket, &messagetoSend, sizeof(messagetoSend), 0, (struct sockaddr *)&nbaddr, sizeof(nbaddr)) < 0) {
		/*cout<<serverList[selfRoutingTable.neighborNode[k]].port<<endl;
		cout<<serverList[selfRoutingTable.neighborNode[k]].ip<<endl;*/
		cerr<<"sendto failed"<<endl;
		//return;
	}
	messagetoSend.numUpdateFields=5;
	return;
}


void counter(int a){
	counter1[getIndex(a)]=1;
	//cout<<"resetting "<<getIndex(a)<<endl;
	return;
}


void applyBellmanFordAlgorithm(){
	if(messageToReceive.numUpdateFields!=1){
		counter(neighbourRoutingTable.selfId);
		recvDataTotal=recvDataTotal+recvData;
	}

	//selfRoutingTable.Cost[selfRoutingTable.selfId][neighbourRoutingTable.selfId]=neighbourRoutingTable.Cost[neighbourRoutingTable.selfId][selfRoutingTable.selfId];
	selfRoutingTable.minCost[selfRoutingTable.selfId][neighbourRoutingTable.selfId]=neighbourRoutingTable.minCost[neighbourRoutingTable.selfId][selfRoutingTable.selfId];
	if(selfRoutingTable.Cost[selfRoutingTable.selfId][neighbourRoutingTable.selfId]==-1000){
		selfRoutingTable.nextHop[selfRoutingTable.selfId][neighbourRoutingTable.selfId]=-1000;
		selfRoutingTable.neighborNode[getIndex(neighbourRoutingTable.selfId)]=-1000;
	}
	//cout<<selfRoutingTable.nextHop[selfRoutingTable.selfId][neighbourRoutingTable.selfId];

	for(int c=1;c<6;c++){
		//selfRoutingTable.Cost[neighbourRoutingTable.selfId][c]=neighbourRoutingTable.Cost[neighbourRoutingTable.selfId][c];
		selfRoutingTable.minCost[neighbourRoutingTable.selfId][c]=neighbourRoutingTable.minCost[neighbourRoutingTable.selfId][c];
	}
	for(int c=1;c<6;c++){
		if((selfRoutingTable.nextHop[selfRoutingTable.selfId][c]==neighbourRoutingTable.selfId)&& (selfRoutingTable.minCost[selfRoutingTable.selfId][neighbourRoutingTable.selfId]!=-1000)&&(selfRoutingTable.minCost[neighbourRoutingTable.selfId][c]!=-1000))
			selfRoutingTable.minCost[selfRoutingTable.selfId][c]=selfRoutingTable.minCost[selfRoutingTable.selfId][neighbourRoutingTable.selfId]+selfRoutingTable.minCost[neighbourRoutingTable.selfId][c];

		else if ((selfRoutingTable.nextHop[selfRoutingTable.selfId][c]==neighbourRoutingTable.selfId)&& ((selfRoutingTable.minCost[selfRoutingTable.selfId][neighbourRoutingTable.selfId]==-1000)||(selfRoutingTable.minCost[neighbourRoutingTable.selfId][c]==-1000)))
		{
			selfRoutingTable.minCost[selfRoutingTable.selfId][c]=-1000;
			selfRoutingTable.nextHop[selfRoutingTable.selfId][c]=-1000;

		}
	}
	for(int j=1;j<6;j++){
		if(selfRoutingTable.selfId!=j){
		//	cout<<neighbourRoutingTable.selfId<<" "<<j<<neighbourRoutingTable.minCost[neighbourRoutingTable.selfId][j]<<endl;
			if(neighbourRoutingTable.minCost[neighbourRoutingTable.selfId][j]!=-1000)
			{
				//cout<<"selfRoutingTable.minCost[selfRoutingTable.selfId][j]==-1000)"<<selfRoutingTable.minCost[selfRoutingTable.selfId][j]<<endl;
				if((selfRoutingTable.minCost[selfRoutingTable.selfId][j]==-1000)||((selfRoutingTable.minCost[selfRoutingTable.selfId][j]) > (selfRoutingTable.minCost[neighbourRoutingTable.selfId][selfRoutingTable.selfId] + selfRoutingTable.minCost[neighbourRoutingTable.selfId][j]))){

					if((selfRoutingTable.nextHop[selfRoutingTable.selfId][neighbourRoutingTable.selfId]==-1000) || selfRoutingTable.nextHop[selfRoutingTable.selfId][neighbourRoutingTable.selfId]==neighbourRoutingTable.selfId){
						selfRoutingTable.nextHop[selfRoutingTable.selfId][j]=neighbourRoutingTable.selfId;}
					selfRoutingTable.minCost[selfRoutingTable.selfId][j]=selfRoutingTable.minCost[selfRoutingTable.selfId][neighbourRoutingTable.selfId] + selfRoutingTable.minCost[neighbourRoutingTable.selfId][j];
					//	cout<<"selfRoutingTable.minCost[selfRoutingTable.selfId][j] "<<selfRoutingTable.minCost[selfRoutingTable.selfId][j]<<endl;
				}
			}
		}
	}
	//cout<<"-BellmanFord SUCCESS"<<endl;
	return;
}


void displayRoutingTable(routingTable routingTableStruct){
	int i=routingTableStruct.selfId;
	cout<<"Destination NextHop Cost"<<endl;
	for(int j=1;j<6;j++){
		if((routingTableStruct.nextHop[i][j]!=-1000)&&(routingTableStruct.nextHop[i][j]!=0))
			cout<<j<<"           "<<routingTableStruct.nextHop[i][j]<<"       "<<routingTableStruct.minCost[i][j]<<endl;
		else if (routingTableStruct.nextHop[i][j]==0)
			cout<<j<<"           "<<routingTableStruct.nextHop[i][j]<<"       "<<"0"<<endl;
		else
			cout<<j<<"           "<<"inf"<<"       "<<"inf"<<endl;
	}
	return;
}


void updateRoutingTable(int a,int b,int c){
	//cout<<"Updating routing table: "<<a<<" "<<b<<" "<<c<<" "<<endl;
	int index=getIndex(b);
	recomputeCostMatrix(a,b,c);
	stepRoutingTable1(index);
	if(c==-1000)
		selfRoutingTable.neighborNode[index]=-1000;

	//cout<<"-update SUCCESS"<<endl;
	return;
}

void disableServer(int b){
	//cout<<"Disabling the link to "<<b<<endl;
	selfRoutingTable.Cost[selfRoutingTable.selfId][b]=-1000;
	int index=getIndex(b);
	if(index==-1000)
	{
		cout<<"-disable FAILED,Link does not exist"<<endl;
		return;
	}
	recomputeCostMatrix(selfRoutingTable.selfId,b,-1000);
	selfRoutingTable.neighborNode[index]=-1000;
//	cout<<"-disable SUCCESS "<<selfRoutingTable.neighborNode[index]<<" : "<<index<<endl;
	return;
}

void initializeVariables(char* ipCmd){
	ifstream topology (ipCmd);
	string topoLine;
	int lineNum=1,neighNum=0;
	string task2[4];
	int j=1;

	//	memset((void *)&selfRoutingTable, 0, sizeof(selfRoutingTable));

	for(int i=1;i<6;i++){
		for(int j=1;j<6;j++){
			if(i==j){
				selfRoutingTable.Cost[i][j]=0;
				selfRoutingTable.nextHop[i][j]=0;
				selfRoutingTable.minCost[i][j]=0;
			}
			else{
				selfRoutingTable.Cost[i][j]=-1000;
				selfRoutingTable.nextHop[i][j]=-1000;
				selfRoutingTable.minCost[i][j]=-1000;
			}
		}
	}

	while(getline(topology, topoLine)){
		istringstream iss(topoLine);
		int x,y;
		int i = 0;
		do
		{
			string sub;
			iss >> sub;
			task2[i]=sub;
			i++;
		} while (iss);

		if(lineNum==1){
			x=atoi(task2[0].c_str());
		}
		else if(lineNum==2){
			y=atoi(task2[0].c_str());//-std=c++11
		}
		else if(lineNum<=7){
			serverList[j].serverId=atoi(task2[0].c_str());
			strcpy(serverList[j].ip,task2[1].c_str());
			serverList[j].port=atoi(task2[2].c_str());
			j++;
		}
		else {
			selfRoutingTable.numServers=x;
			selfRoutingTable.numNeighbors=y;
			selfRoutingTable.selfId=atoi(task2[0].c_str());
			selfRoutingTable.neighborNode[neighNum]=atoi(task2[1].c_str());
			selfRoutingTable.Cost[atoi(task2[0].c_str())][atoi(task2[1].c_str())]=atoi(task2[2].c_str());
			selfRoutingTable.minCost[atoi(task2[0].c_str())][atoi(task2[1].c_str())]=atoi(task2[2].c_str());
			selfRoutingTable.nextHop[atoi(task2[0].c_str())][atoi(task2[1].c_str())]=atoi(task2[1].c_str());
			neighNum++;
		}
		lineNum++;

	}
	topology.close();
	return;
}
string userReq,userReq1[5];
void handleUserInput(){
	getline(cin,userReq);
	istringstream iss(userReq);
	int i = 0;
	do
	{
		string sub;
		iss >> sub;
		userReq1[i]=sub;
		i++;
	} while (iss);

	if (userReq1[0]=="update"){
		if (userReq1[3]=="inf"){
			updateRoutingTable(atoi(userReq1[1].c_str()),atoi(userReq1[2].c_str()),-1000);
		}
		else{
			updateRoutingTable(atoi(userReq1[1].c_str()),atoi(userReq1[2].c_str()),atoi(userReq1[3].c_str()));
		}
		/*else
		{
			cerr<<"incorrect arguments";
		}*/
			cout<<userReq1[0]<<" SUCCESS"<<endl;
	}
	else if (userReq1[0]=="step"){
		stepRoutingTable();
		cout<<userReq1[0]<<" SUCCESS"<<endl;
	}
	else if (userReq1[0]=="packets"){
		cout<<recvDataTotal<<endl;
		recvDataTotal=0;
		cout<<userReq1[0]<<" SUCCESS"<<endl;
	}
	else if (userReq1[0]=="display"){
		displayRoutingTable(selfRoutingTable);
		cout<<userReq1[0]<<" SUCCESS"<<endl;
	}
	else if (userReq1[0]=="disable"){
		disableServer(atoi(userReq1[1].c_str()));
		cout<<userReq1[0]<<" SUCCESS"<<endl;
	}
	else if (userReq1[0]=="crash")
	{
		shutdown(listener_socket,SHUT_RDWR);
		cout<<userReq1[0]<<" SUCCESS"<<endl;
		exit(0);
	}
	else
		cout<<"Invalid Command by user. Please use the following commands: \n1.update\n2.step\n3.packets\n4.display\n5.disable<server-ID>\n6.crash"<<endl;
	return;
}




struct sockaddr_in myaddr;
void startServer(){
	memset((void *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family=AF_INET;
	myaddr.sin_port= htons(serverList[selfRoutingTable.selfId].port);
	//cout<<serverList[selfRoutingTable.selfId].ip<<endl;
	inet_pton(AF_INET,serverList[selfRoutingTable.selfId].ip,&(myaddr.sin_addr));
	if((listener_socket=socket(AF_INET,SOCK_DGRAM,0))<0){
		cerr<<"Socket Error"<<endl;
		return;
	}
	if(bind(listener_socket,(struct sockaddr *)&myaddr,sizeof(myaddr))==-1) {
		cerr<<"Bind Error"<<endl;
		exit(0);
	}
	maxfd=listener_socket;
	return;
}
struct sockaddr_storage addr1;
socklen_t fromlen;


void getneighbourDetails(){
	//messageToReceive.s=inet_ntop(AF_INET,serverList[selfRoutingTable.neighborNode[k]].ip,&(sab.sin_addr));
	for(int i=1;i<6;i++){
		if(strcmp(serverList[i].ip,messageToReceive.serverIP)==0)
			neighbourRoutingTable.selfId=serverList[i].serverId;
	}
	for(int i=1;i<6;i++){
	neighbourRoutingTable.minCost[neighbourRoutingTable.selfId][i]=messageToReceive.cost[i-1];
	}
}

void serverStart(){
	startServer();
	FD_ZERO(&master);
	FD_ZERO(&readfds);
	FD_SET(STDIN_DESC,&master);
	FD_SET(listener_socket,&master);
	for(;;)
	{
		readfds=master;
		if(select(maxfd+1,&readfds,NULL,NULL,NULL)==-1);
		//cerr<<"Select error"<<endl;
		else
		{
			for(fd=0;fd<=maxfd;fd++){
				if(FD_ISSET(fd,&readfds)){
					if(fd==STDIN_DESC){
						handleUserInput();}
					else if(fd==listener_socket){
						fromlen = sizeof addr1;

						//recvfrom(listener_socket, &neighbourRoutingTable, sizeof neighbourRoutingTable, 0,(sockaddr*) &addr1, &fromlen);
						recvData = recvfrom(listener_socket, &messageToReceive, sizeof messageToReceive, 0,(sockaddr*) &addr1, &fromlen);
						recvData=recvData-72;//subtracting the data as per the requirement for easier verification of data
						getneighbourDetails();
						if(getIndex(neighbourRoutingTable.selfId)!=-1000){
							cout<<"RECEIVED A MESSAGE FROM SERVER "<<neighbourRoutingTable.selfId<<endl;
							applyBellmanFordAlgorithm();
						//	displayRoutingTable(selfRoutingTable);
							}
						else
							cout<<"Cannot receive message from other than neighbor node"<<endl;
					}
					else
						cout<<fd<<endl;
				}
			}
		}
	}
}


void setTimer(){
	struct itimerval it_val;

	if (signal(SIGALRM, (void (*)(int)) timerMethods) == SIG_ERR) {
		cerr<<"Unable to catch SIGALRM"<<endl;
		exit(1);
	}

	it_val.it_value.tv_sec =    updateInterval;
	it_val.it_value.tv_usec =   (updateInterval) % 1000000;
	it_val.it_interval = it_val.it_value;
	if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
		cerr<<"error calling setitimer()";
		exit(1);
	}
	return;
}


int main(int argc, char* ipCmd[]){
	if(strcmp(ipCmd[2],"-t")!=0||strcmp(ipCmd[4],"-i")!=0){
		cout<<"Enter the  correct arguments in the form of -serverList -t <topology-file-name> -i <routing-update-interval>"<<endl;
		exit(EXIT_FAILURE);
	}
	updateInterval=atoi(ipCmd[5]);
	initializeVariables(ipCmd[3]);
	setTimer();

	//displayRoutingTable(selfRoutingTable);
	serverStart();
	return 0;
}

