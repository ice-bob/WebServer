#ifndef SQL_CONNECTION_H
#define SQL_CONNECTION_H

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../locker/locker.h"

using namespace std;

class connection_pool{
private:
	unsigned int MaxConn; //最大连接数
	unsigned int CurConn; //当前已用连接数
	unsigned int FreeConn;//空闲连接数

	locker lock;
	list<MYSQL*> connList;
	sem reserve;


	string url;
	int port;
	string user;
	string password;
	string databasename;

public:
	MYSQL* GetConnection();
	bool ReleaseConnection(MYSQL* conn);
	int GetFreeConn();
	void DestroyPool();

	static connection_pool *GetInstance();

	void init(string url, string user, string password, string databasename, int port, unsigned int MaxConn);

	connection_pool();
	~connection_pool();
};

class connectionRAII{
private:
	MYSQL* conRAII;
	connection_pool *poolRAII;
public:
	connectionRAII(MYSQL** con, connection_pool* connPool);
	~connectionRAII();
};

#endif