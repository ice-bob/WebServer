#ifndef LST_TIMER_H
#define LST_TIMER_H

#include <time.h>

#include "../log/log.h"

class util_timer;
struct client_data{
	sockaddr_in address;
	int sockfd;
	util_timer *timer;
};

class util_timer{
public:
	util_timer():prev(NULL),next(NULL){}

	time_t expire;
	void (*cb_func)(client_data *);
	client_data* user_data;
	util_timer *prev;
	util_timer *next;
};


class sort_timer_lst{
private:
	util_timer *head;
	util_timer *tail;

	void add_timer(util_timer* timer, util_timer *lst_head){
		util_timer* prev = lst_head;
		util_timer* tmp = prev->next;
		while(tmp){
			if(timer->expire < tmp->expire){
				prev->next = timer;
				timer->next = tmp;
				tmp->prev = timer;
				timer->prev = prev;
				break;
			}
			prev = tmp;
			tmp = tmp->next;
		}
		if(!tmp){
			prev->next = timer;
			timer->prev = prev;
			timer->next = NULL;
			tail = timer;
		}
	}

public:
	sort_timer_lst():head(NULL),tail(NULL){}
	~sort_timer_lst(){
		util_timer *tmp = head;
		while(tmp){
			head = tmp->next;
			delete tmp;
			tmp = head;
		}
	}

	bool add_timer(util_timer* timer){
		if(!timer) return false;
		if(!head){
			head = tail = timer;
			return true;
		}
		if(timer->expire < head->expire){
			timer->next = head;
			head->prev = timer;
			head = timer;
			return true;
		}
		add_timer(timer,head);
		return true;
	}

	void adjust_timer(util_timer *timer){
		if(!timer){
			return;
		}
		util_timer *tmp = timer->next;
		if(!tmp || (timer->expire < tmp->expire)){
			return;
		}
		if(timer == head){
			head = head->next;
			head->prev = NULL;
			timer->next = NULL;
			add_timer(timer,head);
		}
		else{
			timer->prev->next = timer->next;
			timer->next->prev = timer->prev;
			add_timer(timer,timer->next);
		}
	}

	void del_timer(util_timer* timer){
		if(!timer) return;
		if((timer == head) && (timer == tail)){
			head = NULL;
			tail = NULL;
		}
		else if(timer == head){
			head = head->next;
			head->prev = NULL;
		}
		else if(timer == tail){
			tail = tail->prev;
			tail->next = NULL;
		}
		else{
			timer->prev->next = timer->next;
			timer->next->prev = timer->prev;
		}
		delete timer;
	}

	void tick(){
		if(!head) return;
		LOG_INFO("%s","timer tick");
		Log::get_instance()->flush();
		time_t cur = time(NULL);
		util_timer* tmp = head;
		while(tmp){
			if(cur<tmp->expire){
				break;
			}
			tmp->cb_func(tmp->user_data);
			head = tmp->next;
			if(head) head->prev = NULL;

			delete tmp;
			tmp = head;
		}
	}
};



#endif