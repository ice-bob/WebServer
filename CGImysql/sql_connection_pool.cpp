#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>

#include "../log/log.h"
#include "sql_connection_pool.h"

using namespace std;

connection_pool::connection_pool(){
	this->CurConn = 0;
	this->FreeConn = 0;
}

connection_pool::~connection_pool(){
	DestroyPool();
}

connection_pool* connection_pool::GetInstance(){
	static connection_pool connPool;
	return &connPool;
}

void connection_pool::init(string url, string user, string password, 
						   string databasename, int port, unsigned int MaxConn){
	this->url = url;
	this->port = port;
	this->user = user;
	this->password = password;
	this->databasename = databasename;

	lock.lock();
	for(int i=0;i<MaxConn;i++){
		MYSQL *con = NULL;
		con = mysql_init(con);
		if(con == NULL){
			LOG_ERROR("MYSQL CONNECTION error:%s\n", mysql_error(con));
			Log::get_instance()->flush();
			exit(1);
		}
		con = mysql_real_connect(con,url.c_str(),user.c_str(),password.c_str(),databasename.c_str(),port,NULL,0);
		if(con == NULL){
			LOG_ERROR("MYSQL CONNECTION error:%s\n", mysql_error(con));
			Log::get_instance()->flush();
			exit(1);
		}
		connList.push_back(con);
		++FreeConn;
	}

	reserve = sem(FreeConn);
	this->MaxConn = FreeConn;

	lock.unlock();
}

MYSQL *connection_pool::GetConnection(){
	MYSQL* con = NULL;

	if(connList.empty()) return NULL;

	reserve.wait();

	lock.lock();
	con = connList.front();
	connList.pop_front();

	--FreeConn;
	++CurConn;

	lock.unlock();
	return con;
}

bool connection_pool::ReleaseConnection(MYSQL* con){
	if(con == NULL) return false;
	lock.lock();
	connList.push_back(con);
	++FreeConn;
	--CurConn;
	lock.unlock();
	reserve.post();
	return true;;
}

void connection_pool::DestroyPool(){
	lock.lock();
	for(auto it = connList.begin();it!=connList.end();it++){
		mysql_close(*it);
	}
	CurConn = 0;
	FreeConn = 0;
	if(!connList.empty()) connList.clear();

	lock.unlock();
}

int connection_pool::GetFreeConn(){
	return this->FreeConn;
}

connectionRAII::connectionRAII(MYSQL** SQL, connection_pool* connPool){
	*SQL = connPool->GetConnection();

	conRAII = *SQL;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}