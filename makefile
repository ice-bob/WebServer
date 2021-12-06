server: main.c ./threadpool/threadpool.h ./http_conn/http_conn.cpp ./http_conn/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./log/block_queue.h ./redis/redis.cpp ./redis/redis.h
	g++ -o server main.c ./threadpool/threadpool.h ./http_conn/http_conn.cpp ./http_conn/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./redis/redis.cpp ./redis/redis.h -lpthread -lmysqlclient -lhiredis


clean:
	rm  -r server
