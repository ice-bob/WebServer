#ifndef REDIS_H
#define REDIS_H

#include <iostream>
#include <list>
#include <string>
#include <stdio.h>
#include <hiredis/hiredis.h>

#include "../locker/locker.h"

using namespace std;

class redis_pool{
private:
	unsigned int MaxConn;
	unsigned int CurConn;
	unsigned int FreeConn;

	locker lock;
	sem reserve;

	list<redisContext*> connList;
//	redisReply* r_reply;

	string host;
	int port;
	
public:
	redisContext* GetConnection();
	bool ReleaseConnection(redisContext* r_conn);
	int GetFreeConn();
	void DestroyPool();

	static redis_pool* GetInstance();

	void init(string host, int port, unsigned int MaxConn);

	redis_pool();
	~redis_pool();
};

class connectionRAII{
private:
	redisContext* conRAII;
	redis_pool* poolRAII;
public:
	connectionRAII(redisContext** con, redis_pool* connPool);
	~connectionRAII();
};


#endif