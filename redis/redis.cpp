#include <iostream>
#include <list>
#include <string>
#include <stdio.h>
#include <hiredis/hiredis.h>

#include "redis.h"
#include "../log/log.h"
using namespace std;

redis_pool::redis_pool(){
	this->CurConn = 0;
	this->FreeConn = 0;
}

redis_pool::~redis_pool(){
	DestroyPool();
}

redis_pool* redis_pool::GetInstance(){
	static redis_pool r_pool;
	return &r_pool;
}

void redis_pool::init(string host, int port, unsigned int MaxConn){
	this->host = host;
	this->port = port;
	
	lock.lock();
	for(int i=0;i<MaxConn;i++){
		redisContext* r_conn = NULL;
		r_conn = redisConnect(host.c_str(),port);
		if(r_conn != NULL && r_conn->err){
			LOG_ERROR("REDIS CONNECT ERROR:%s\n",r_conn->errstr);
			Log::get_instance()->flush();
			exit(1);
		}
		connList.push_back(r_conn);
		++FreeConn;
	}
	reserve = sem(FreeConn);
	this->MaxConn = FreeConn;
	lock.unlock();
}

redisContext* redis_pool::GetConnection(){
	redisContext* r_conn = NULL;
	if(connList.empty()) return NULL;

	reserve.wait();
	lock.lock();
	r_conn = connList.front();
	connList.pop_front();

	--FreeConn;
	++CurConn;

	lock.unlock();
	return r_conn;
}

bool redis_pool::ReleaseConnection(redisContext* r_conn){
	if(r_conn == NULL) return false;
	lock.lock();
	connList.push_back(r_conn);
	++FreeConn;
	--CurConn;
	lock.unlock();
	reserve.post();
	return true;
}

void redis_pool::DestroyPool(){
	lock.lock();
	for(auto it=connList.begin();it!=connList.end();it++){
		redisFree(*it);
	}
	FreeConn = 0;
	CurConn = 0;
	if(!connList.empty()) connList.clear();

	lock.unlock();
}

int redis_pool::GetFreeConn(){
	return this->FreeConn;
}

connectionRAII::connectionRAII(redisContext** con, redis_pool* connPool){
	*con = connPool->GetConnection();

	conRAII = *con;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}

